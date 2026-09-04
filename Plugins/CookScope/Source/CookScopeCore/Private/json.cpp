#include "cookscope/json.h"

#include <array>
#include <charconv>
#include <utility>

namespace cookscope
{
	namespace
	{
		class JsonParser
		{
		public:
			JsonParser(std::string_view input, std::size_t maximumDepth)
				: Input(input), MaximumDepth(maximumDepth)
			{
			}

			JsonParseResult Parse()
			{
				JsonParseResult result;
				SkipWhitespace();
				if (!ParseValue(result.value, "$", 0))
				{
					result.error = std::move(Error);
					return result;
				}
				SkipWhitespace();
				if (Cursor != Input.size())
				{
					Fail(JsonErrorCode::TrailingContent, "$", "unexpected content after the root value");
					result.error = std::move(Error);
					return result;
				}
				result.ok = true;
				return result;
			}

		private:
			bool ParseValue(JsonValue& output, const std::string& path, std::size_t depth)
			{
				if (depth > MaximumDepth)
				{
					return Fail(JsonErrorCode::DepthExceeded, path, "maximum JSON depth exceeded");
				}
				SkipWhitespace();
				if (Cursor >= Input.size())
				{
					return Fail(JsonErrorCode::UnexpectedEnd, path, "expected a JSON value");
				}

				switch (Input[Cursor])
				{
				case 'n':
					if (!ConsumeLiteral("null", path))
					{
						return false;
					}
					output.type = JsonType::Null;
					return true;
				case 't':
					if (!ConsumeLiteral("true", path))
					{
						return false;
					}
					output.type = JsonType::Boolean;
					output.boolean = true;
					return true;
				case 'f':
					if (!ConsumeLiteral("false", path))
					{
						return false;
					}
					output.type = JsonType::Boolean;
					output.boolean = false;
					return true;
				case '"':
					output.type = JsonType::String;
					return ParseString(output.scalar, path);
				case '[':
					return ParseArray(output, path, depth);
				case '{':
					return ParseObject(output, path, depth);
				default:
					if (Input[Cursor] == '-' || IsDigit(Input[Cursor]))
					{
						output.type = JsonType::Number;
						return ParseNumber(output.scalar, path);
					}
					return Fail(JsonErrorCode::UnexpectedToken, path, "unexpected token while parsing JSON value");
				}
			}

			bool ParseArray(JsonValue& output, const std::string& path, std::size_t depth)
			{
				++Cursor;
				output.type = JsonType::Array;
				SkipWhitespace();
				if (Consume(']'))
				{
					return true;
				}

				std::size_t index = 0;
				while (true)
				{
					JsonValue child;
					const std::string childPath = path + "[" + std::to_string(index) + "]";
					if (!ParseValue(child, childPath, depth + 1))
					{
						return false;
					}
					output.array.push_back(std::move(child));
					SkipWhitespace();
					if (Consume(']'))
					{
						return true;
					}
					if (!Consume(','))
					{
						return Fail(JsonErrorCode::UnexpectedToken, path, "expected ',' or ']' in array");
					}
					++index;
				}
			}

			bool ParseObject(JsonValue& output, const std::string& path, std::size_t depth)
			{
				++Cursor;
				output.type = JsonType::Object;
				SkipWhitespace();
				if (Consume('}'))
				{
					return true;
				}

				while (true)
				{
					SkipWhitespace();
					if (Cursor >= Input.size() || Input[Cursor] != '"')
					{
						return Fail(JsonErrorCode::UnexpectedToken, path, "expected string key in object");
					}
					std::string key;
					if (!ParseString(key, path))
					{
						return false;
					}
					const std::string childPath = path == "$" ? "$." + key : path + "." + key;
					if (output.object.contains(key))
					{
						return Fail(JsonErrorCode::DuplicateKey, childPath, "duplicate object key");
					}
					SkipWhitespace();
					if (!Consume(':'))
					{
						return Fail(JsonErrorCode::UnexpectedToken, childPath, "expected ':' after object key");
					}
					JsonValue child;
					if (!ParseValue(child, childPath, depth + 1))
					{
						return false;
					}
					output.object.emplace(std::move(key), std::move(child));
					SkipWhitespace();
					if (Consume('}'))
					{
						return true;
					}
					if (!Consume(','))
					{
						return Fail(JsonErrorCode::UnexpectedToken, path, "expected ',' or '}' in object");
					}
				}
			}

			bool ParseString(std::string& output, const std::string& path)
			{
				++Cursor;
				while (Cursor < Input.size())
				{
					const unsigned char character = static_cast<unsigned char>(Input[Cursor++]);
					if (character == '"')
					{
						return true;
					}
					if (character < 0x20)
					{
						return Fail(JsonErrorCode::InvalidString, path, "unescaped control character in string");
					}
					if (character == '\\')
					{
						if (!ParseEscape(output, path))
						{
							return false;
						}
						continue;
					}
					if (character < 0x80)
					{
						output.push_back(static_cast<char>(character));
						continue;
					}
					if (!ParseRawUtf8(character, output, path))
					{
						return false;
					}
				}
				return Fail(JsonErrorCode::UnexpectedEnd, path, "unterminated string");
			}

			bool ParseEscape(std::string& output, const std::string& path)
			{
				if (Cursor >= Input.size())
				{
					return Fail(JsonErrorCode::UnexpectedEnd, path, "unterminated escape sequence");
				}
				const char escape = Input[Cursor++];
				switch (escape)
				{
				case '"': output.push_back('"'); return true;
				case '\\': output.push_back('\\'); return true;
				case '/': output.push_back('/'); return true;
				case 'b': output.push_back('\b'); return true;
				case 'f': output.push_back('\f'); return true;
				case 'n': output.push_back('\n'); return true;
				case 'r': output.push_back('\r'); return true;
				case 't': output.push_back('\t'); return true;
				case 'u': return ParseUnicodeEscape(output, path);
				default: return Fail(JsonErrorCode::InvalidEscape, path, "invalid string escape");
				}
			}

			bool ParseUnicodeEscape(std::string& output, const std::string& path)
			{
				std::uint32_t first = 0;
				if (!ReadHexQuad(first, path))
				{
					return false;
				}
				std::uint32_t codePoint = first;
				if (first >= 0xD800 && first <= 0xDBFF)
				{
					if (Cursor + 2 > Input.size() || Input[Cursor] != '\\' || Input[Cursor + 1] != 'u')
					{
						return Fail(JsonErrorCode::InvalidUnicode, path, "high surrogate is not followed by a low surrogate");
					}
					Cursor += 2;
					std::uint32_t second = 0;
					if (!ReadHexQuad(second, path) || second < 0xDC00 || second > 0xDFFF)
					{
						return Fail(JsonErrorCode::InvalidUnicode, path, "invalid low surrogate");
					}
					codePoint = 0x10000 + ((first - 0xD800) << 10) + (second - 0xDC00);
				}
				else if (first >= 0xDC00 && first <= 0xDFFF)
				{
					return Fail(JsonErrorCode::InvalidUnicode, path, "unpaired low surrogate");
				}
				AppendCodePoint(codePoint, output);
				return true;
			}

			bool ReadHexQuad(std::uint32_t& value, const std::string& path)
			{
				if (Cursor + 4 > Input.size())
				{
					return Fail(JsonErrorCode::UnexpectedEnd, path, "incomplete Unicode escape");
				}
				value = 0;
				for (int index = 0; index < 4; ++index)
				{
					const char digit = Input[Cursor++];
					value <<= 4;
					if (digit >= '0' && digit <= '9') value += static_cast<std::uint32_t>(digit - '0');
					else if (digit >= 'a' && digit <= 'f') value += static_cast<std::uint32_t>(digit - 'a' + 10);
					else if (digit >= 'A' && digit <= 'F') value += static_cast<std::uint32_t>(digit - 'A' + 10);
					else return Fail(JsonErrorCode::InvalidUnicode, path, "non-hexadecimal Unicode escape");
				}
				return true;
			}

			bool ParseRawUtf8(unsigned char first, std::string& output, const std::string& path)
			{
				int continuationCount = 0;
				std::uint32_t codePoint = 0;
				std::uint32_t minimum = 0;
				if ((first & 0xE0) == 0xC0) { continuationCount = 1; codePoint = first & 0x1F; minimum = 0x80; }
				else if ((first & 0xF0) == 0xE0) { continuationCount = 2; codePoint = first & 0x0F; minimum = 0x800; }
				else if ((first & 0xF8) == 0xF0) { continuationCount = 3; codePoint = first & 0x07; minimum = 0x10000; }
				else return Fail(JsonErrorCode::InvalidUnicode, path, "invalid UTF-8 leading byte");

				output.push_back(static_cast<char>(first));
				for (int index = 0; index < continuationCount; ++index)
				{
					if (Cursor >= Input.size())
					{
						return Fail(JsonErrorCode::UnexpectedEnd, path, "incomplete UTF-8 sequence");
					}
					const unsigned char next = static_cast<unsigned char>(Input[Cursor++]);
					if ((next & 0xC0) != 0x80)
					{
						return Fail(JsonErrorCode::InvalidUnicode, path, "invalid UTF-8 continuation byte");
					}
					codePoint = (codePoint << 6) | (next & 0x3F);
					output.push_back(static_cast<char>(next));
				}
				if (codePoint < minimum || codePoint > 0x10FFFF || (codePoint >= 0xD800 && codePoint <= 0xDFFF))
				{
					return Fail(JsonErrorCode::InvalidUnicode, path, "non-canonical UTF-8 code point");
				}
				return true;
			}

			bool ParseNumber(std::string& output, const std::string& path)
			{
				const std::size_t start = Cursor;
				Consume('-');
				if (Cursor >= Input.size())
				{
					return Fail(JsonErrorCode::InvalidNumber, path, "incomplete number");
				}
				if (Input[Cursor] == '0')
				{
					++Cursor;
					if (Cursor < Input.size() && IsDigit(Input[Cursor]))
					{
						return Fail(JsonErrorCode::InvalidNumber, path, "leading zero in number");
					}
				}
				else if (Input[Cursor] >= '1' && Input[Cursor] <= '9')
				{
					while (Cursor < Input.size() && IsDigit(Input[Cursor])) ++Cursor;
				}
				else
				{
					return Fail(JsonErrorCode::InvalidNumber, path, "expected integer component");
				}

				if (Consume('.'))
				{
					if (Cursor >= Input.size() || !IsDigit(Input[Cursor]))
					{
						return Fail(JsonErrorCode::InvalidNumber, path, "expected fractional digits");
					}
					while (Cursor < Input.size() && IsDigit(Input[Cursor])) ++Cursor;
				}
				if (Cursor < Input.size() && (Input[Cursor] == 'e' || Input[Cursor] == 'E'))
				{
					++Cursor;
					if (Cursor < Input.size() && (Input[Cursor] == '+' || Input[Cursor] == '-')) ++Cursor;
					if (Cursor >= Input.size() || !IsDigit(Input[Cursor]))
					{
						return Fail(JsonErrorCode::InvalidNumber, path, "expected exponent digits");
					}
					while (Cursor < Input.size() && IsDigit(Input[Cursor])) ++Cursor;
				}
				output.assign(Input.substr(start, Cursor - start));
				return true;
			}

			bool ConsumeLiteral(std::string_view literal, const std::string& path)
			{
				if (Input.substr(Cursor, literal.size()) != literal)
				{
					return Fail(JsonErrorCode::UnexpectedToken, path, "invalid JSON literal");
				}
				Cursor += literal.size();
				return true;
			}

			bool Consume(char expected)
			{
				if (Cursor < Input.size() && Input[Cursor] == expected)
				{
					++Cursor;
					return true;
				}
				return false;
			}

			void SkipWhitespace()
			{
				while (Cursor < Input.size() &&
					(Input[Cursor] == ' ' || Input[Cursor] == '\t' || Input[Cursor] == '\r' || Input[Cursor] == '\n'))
				{
					++Cursor;
				}
			}

			bool Fail(JsonErrorCode code, const std::string& path, std::string message)
			{
				if (Error.code == JsonErrorCode::None)
				{
					Error.code = code;
					Error.offset = Cursor;
					Error.path = path;
					Error.message = std::move(message);
				}
				return false;
			}

			static bool IsDigit(char value) noexcept
			{
				return value >= '0' && value <= '9';
			}

			static void AppendCodePoint(std::uint32_t codePoint, std::string& output)
			{
				if (codePoint <= 0x7F)
				{
					output.push_back(static_cast<char>(codePoint));
				}
				else if (codePoint <= 0x7FF)
				{
					output.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
					output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
				}
				else if (codePoint <= 0xFFFF)
				{
					output.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
					output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
					output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
				}
				else
				{
					output.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
					output.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
					output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
					output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
				}
			}

			std::string_view Input;
			std::size_t MaximumDepth = 0;
			std::size_t Cursor = 0;
			JsonError Error;
		};

		void WriteEscapedString(std::string_view value, std::string& output)
		{
			constexpr std::array<char, 16> Hex = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
			output.push_back('"');
			for (const unsigned char character : value)
			{
				switch (character)
				{
				case '"': output += "\\\""; break;
				case '\\': output += "\\\\"; break;
				case '\b': output += "\\b"; break;
				case '\f': output += "\\f"; break;
				case '\n': output += "\\n"; break;
				case '\r': output += "\\r"; break;
				case '\t': output += "\\t"; break;
				default:
					if (character < 0x20)
					{
						output += "\\u00";
						output.push_back(Hex[(character >> 4) & 0x0F]);
						output.push_back(Hex[character & 0x0F]);
					}
					else
					{
						output.push_back(static_cast<char>(character));
					}
					break;
				}
			}
			output.push_back('"');
		}

		void WriteValue(const JsonValue& value, std::string& output)
		{
			switch (value.type)
			{
			case JsonType::Null:
				output += "null";
				break;
			case JsonType::Boolean:
				output += value.boolean ? "true" : "false";
				break;
			case JsonType::Number:
				output += value.scalar;
				break;
			case JsonType::String:
				WriteEscapedString(value.scalar, output);
				break;
			case JsonType::Array:
				output.push_back('[');
				for (std::size_t index = 0; index < value.array.size(); ++index)
				{
					if (index != 0) output.push_back(',');
					WriteValue(value.array[index], output);
				}
				output.push_back(']');
				break;
			case JsonType::Object:
				output.push_back('{');
				{
					bool first = true;
					for (const auto& [key, child] : value.object)
					{
						if (!first) output.push_back(',');
						first = false;
						WriteEscapedString(key, output);
						output.push_back(':');
						WriteValue(child, output);
					}
				}
				output.push_back('}');
				break;
			}
		}
	}

	JsonParseResult ParseJson(std::string_view utf8Json, std::size_t maximumDepth)
	{
		return JsonParser(utf8Json, maximumDepth).Parse();
	}

	std::string WriteCanonicalJson(const JsonValue& value)
	{
		std::string output;
		WriteValue(value, output);
		return output;
	}
}
