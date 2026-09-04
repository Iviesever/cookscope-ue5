#include "cookscope/reports.h"

#include "cookscope/json.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace cookscope
{
	namespace
	{
		JsonValue ReportJsonString(std::string value)
		{
			JsonValue result;
			result.type = JsonType::String;
			result.scalar = std::move(value);
			return result;
		}

		JsonValue ReportJsonNumber(std::uint64_t value)
		{
			JsonValue result;
			result.type = JsonType::Number;
			result.scalar = std::to_string(value);
			return result;
		}

		JsonValue ReportJsonBoolean(bool value)
		{
			JsonValue result;
			result.type = JsonType::Boolean;
			result.boolean = value;
			return result;
		}

		std::string ReportSeverityName(Severity severity)
		{
			switch (severity)
			{
			case Severity::Note: return "note";
			case Severity::Warning: return "warning";
			case Severity::Error: return "error";
			}
			return "error";
		}

		std::string ReportMeasurementName(MeasurementKind kind)
		{
			switch (kind)
			{
			case MeasurementKind::SourceDisk: return "source-disk";
			case MeasurementKind::PackageDisk: return "package-disk";
			case MeasurementKind::Estimated: return "estimated";
			case MeasurementKind::ActualCooked: return "actual-cooked";
			case MeasurementKind::Unavailable: return "unavailable";
			}
			return "unavailable";
		}

		std::string ReportDependencyName(DependencyKind kind)
		{
			switch (kind)
			{
			case DependencyKind::Hard: return "hard";
			case DependencyKind::Soft: return "soft";
			case DependencyKind::Manage: return "manage";
			case DependencyKind::SearchableName: return "searchable-name";
			}
			return "hard";
		}

		std::string ReportBaselineName(FindingBaselineState state)
		{
			switch (state)
			{
			case FindingBaselineState::NotApplicable: return "not-applicable";
			case FindingBaselineState::New: return "new";
			case FindingBaselineState::Existing: return "existing";
			case FindingBaselineState::Worsened: return "worsened";
			}
			return "not-applicable";
		}

		JsonValue OptionalNumber(const std::optional<std::uint64_t>& value)
		{
			return value ? ReportJsonNumber(*value) : JsonValue{};
		}

		JsonValue FindingJson(const Finding& finding)
		{
			JsonValue value;
			value.type = JsonType::Object;
			value.object.emplace("ruleId", ReportJsonString(finding.ruleId));
			value.object.emplace("assetPath", ReportJsonString(finding.assetPath));
			value.object.emplace("relatedAsset", ReportJsonString(finding.relatedAsset));
			value.object.emplace("severity", ReportJsonString(ReportSeverityName(finding.severity)));
			value.object.emplace("message", ReportJsonString(finding.message));
			value.object.emplace("measurementKind", ReportJsonString(ReportMeasurementName(finding.measurementKind)));
			value.object.emplace("observedBytes", OptionalNumber(finding.observedBytes));
			value.object.emplace("budgetBytes", OptionalNumber(finding.budgetBytes));
			value.object.emplace("metric", ReportJsonString(finding.metric));
			value.object.emplace("observedValue", OptionalNumber(finding.observedValue));
			value.object.emplace("limitValue", OptionalNumber(finding.limitValue));
			value.object.emplace("observedText", ReportJsonString(finding.observedText));
			value.object.emplace("expectedText", ReportJsonString(finding.expectedText));
			value.object.emplace("baselineState", ReportJsonString(ReportBaselineName(finding.baselineState)));
			value.object.emplace("dependencyKind", finding.dependencyKind
				? ReportJsonString(ReportDependencyName(*finding.dependencyKind)) : JsonValue{});
			JsonValue path;
			path.type = JsonType::Array;
			for (const DependencyStep& step : finding.dependencyPath)
			{
				JsonValue item;
				item.type = JsonType::Object;
				item.object.emplace("source", ReportJsonString(step.source));
				item.object.emplace("target", ReportJsonString(step.target));
				item.object.emplace("kind", ReportJsonString(ReportDependencyName(step.kind)));
				path.array.push_back(std::move(item));
			}
			value.object.emplace("dependencyPath", std::move(path));
			return value;
		}

		JsonValue CanonicalResult(
			const Snapshot& snapshot,
			const RuleConfig& config,
			const AnalysisResult& analysis,
			const SnapshotDiffResult* diff)
		{
			JsonValue root;
			root.type = JsonType::Object;
			root.object.emplace("schema", ReportJsonString("cookscope.result/1"));

			JsonValue provenance;
			provenance.type = JsonType::Object;
			provenance.object.emplace("engineVersion", ReportJsonString(snapshot.provenance.engineVersion));
			provenance.object.emplace("platform", ReportJsonString(snapshot.provenance.platform));
			provenance.object.emplace("cookConfiguration", ReportJsonString(snapshot.provenance.cookConfiguration));
			provenance.object.emplace("sourceSha", ReportJsonString(snapshot.provenance.sourceSha));
			root.object.emplace("provenance", std::move(provenance));

			JsonValue summary;
			summary.type = JsonType::Object;
			summary.object.emplace("assets", ReportJsonNumber(snapshot.assets.size()));
			summary.object.emplace("findings", ReportJsonNumber(analysis.findings.size()));
			summary.object.emplace("diagnostics", ReportJsonNumber(analysis.diagnostics.size()));
			std::uint64_t errors = 0;
			std::uint64_t warnings = 0;
			for (const Finding& finding : analysis.findings)
			{
				if (finding.severity == Severity::Error) ++errors;
				else if (finding.severity == Severity::Warning) ++warnings;
			}
			summary.object.emplace("errors", ReportJsonNumber(errors));
			summary.object.emplace("warnings", ReportJsonNumber(warnings));
			root.object.emplace("summary", std::move(summary));

			JsonValue rules;
			rules.type = JsonType::Array;
			std::vector<RuleDefinition> sortedRules = config.rules;
			std::sort(sortedRules.begin(), sortedRules.end(), [](const RuleDefinition& left, const RuleDefinition& right) {
				return left.id < right.id;
			});
			for (const RuleDefinition& rule : sortedRules)
			{
				JsonValue item;
				item.type = JsonType::Object;
				item.object.emplace("id", ReportJsonString(rule.id));
				item.object.emplace("name", ReportJsonString(rule.name));
				item.object.emplace("description", ReportJsonString(rule.description));
				item.object.emplace("severity", ReportJsonString(ReportSeverityName(rule.severity)));
				item.object.emplace("helpUri", ReportJsonString(rule.helpUri));
				rules.array.push_back(std::move(item));
			}
			root.object.emplace("rules", std::move(rules));

			JsonValue assets;
			assets.type = JsonType::Array;
			std::vector<AssetRecord> sortedAssets = snapshot.assets;
			std::sort(sortedAssets.begin(), sortedAssets.end(), [](const AssetRecord& left, const AssetRecord& right) {
				return left.objectPath < right.objectPath;
			});
			for (const AssetRecord& asset : sortedAssets)
			{
				JsonValue item;
				item.type = JsonType::Object;
				item.object.emplace("objectPath", ReportJsonString(asset.objectPath));
				item.object.emplace("assetClass", ReportJsonString(asset.assetClass));
				item.object.emplace("primaryAssetId", asset.primaryAssetId ? ReportJsonString(*asset.primaryAssetId) : JsonValue{});
				item.object.emplace("cookedSizeKind", ReportJsonString(ReportMeasurementName(asset.cookedSize.kind)));
				item.object.emplace("cookedBytes", OptionalNumber(asset.cookedSize.bytes));
				assets.array.push_back(std::move(item));
			}
			root.object.emplace("assets", std::move(assets));

			JsonValue findings;
			findings.type = JsonType::Array;
			for (const Finding& finding : analysis.findings) findings.array.push_back(FindingJson(finding));
			root.object.emplace("findings", std::move(findings));

			JsonValue diagnostics;
			diagnostics.type = JsonType::Array;
			for (const AnalysisDiagnostic& diagnostic : analysis.diagnostics)
			{
				JsonValue item;
				item.type = JsonType::Object;
				item.object.emplace("ruleId", ReportJsonString(diagnostic.ruleId));
				item.object.emplace("assetPath", ReportJsonString(diagnostic.assetPath));
				item.object.emplace("code", ReportJsonNumber(static_cast<std::uint64_t>(diagnostic.code)));
				item.object.emplace("message", ReportJsonString(diagnostic.message));
				diagnostics.array.push_back(std::move(item));
			}
			root.object.emplace("diagnostics", std::move(diagnostics));

			if (diff)
			{
				JsonParseResult parsedDiff = ParseJson(WriteCanonicalDiff(*diff));
				root.object.emplace("diff", parsedDiff.ok ? std::move(parsedDiff.value) : JsonValue{});
			}
			else
			{
				root.object.emplace("diff", JsonValue{});
			}
			return root;
		}

		std::string RenderSarif(const RuleConfig& config, const AnalysisResult& analysis)
		{
			JsonValue root;
			root.type = JsonType::Object;
			root.object.emplace("$schema", ReportJsonString("https://json.schemastore.org/sarif-2.1.0.json"));
			root.object.emplace("version", ReportJsonString("2.1.0"));
			JsonValue runs;
			runs.type = JsonType::Array;
			JsonValue run;
			run.type = JsonType::Object;
			JsonValue tool;
			tool.type = JsonType::Object;
			JsonValue driver;
			driver.type = JsonType::Object;
			driver.object.emplace("name", ReportJsonString("CookScope"));
			driver.object.emplace("semanticVersion", ReportJsonString("0.1.0"));
			JsonValue rules;
			rules.type = JsonType::Array;
			for (const RuleDefinition& rule : config.rules)
			{
				JsonValue item;
				item.type = JsonType::Object;
				item.object.emplace("id", ReportJsonString(rule.id));
				JsonValue shortDescription;
				shortDescription.type = JsonType::Object;
				shortDescription.object.emplace("text", ReportJsonString(rule.name));
				item.object.emplace("shortDescription", std::move(shortDescription));
				JsonValue fullDescription;
				fullDescription.type = JsonType::Object;
				fullDescription.object.emplace("text", ReportJsonString(rule.description));
				item.object.emplace("fullDescription", std::move(fullDescription));
				JsonValue helpUri;
				helpUri.type = JsonType::String;
				helpUri.scalar = rule.helpUri;
				item.object.emplace("helpUri", std::move(helpUri));
				rules.array.push_back(std::move(item));
			}
			driver.object.emplace("rules", std::move(rules));
			tool.object.emplace("driver", std::move(driver));
			run.object.emplace("tool", std::move(tool));

			JsonValue results;
			results.type = JsonType::Array;
			for (const Finding& finding : analysis.findings)
			{
				JsonValue item;
				item.type = JsonType::Object;
				item.object.emplace("ruleId", ReportJsonString(finding.ruleId));
				item.object.emplace("level", ReportJsonString(ReportSeverityName(finding.severity)));
				JsonValue message;
				message.type = JsonType::Object;
				message.object.emplace("text", ReportJsonString(finding.message));
				item.object.emplace("message", std::move(message));
				JsonValue locations;
				locations.type = JsonType::Array;
				JsonValue location;
				location.type = JsonType::Object;
				JsonValue physical;
				physical.type = JsonType::Object;
				JsonValue artifact;
				artifact.type = JsonType::Object;
				artifact.object.emplace("uri", ReportJsonString(finding.assetPath));
				physical.object.emplace("artifactLocation", std::move(artifact));
				location.object.emplace("physicalLocation", std::move(physical));
				locations.array.push_back(std::move(location));
				item.object.emplace("locations", std::move(locations));
				results.array.push_back(std::move(item));
			}
			run.object.emplace("results", std::move(results));
			runs.array.push_back(std::move(run));
			root.object.emplace("runs", std::move(runs));
			return WriteCanonicalJson(root) + "\n";
		}

		std::string EscapeXml(std::string_view value)
		{
			std::string output;
			for (const char character : value)
			{
				switch (character)
				{
				case '&': output += "&amp;"; break;
				case '<': output += "&lt;"; break;
				case '>': output += "&gt;"; break;
				case '"': output += "&quot;"; break;
				case '\'': output += "&apos;"; break;
				default: output.push_back(character); break;
				}
			}
			return output;
		}

		std::string RenderJUnit(const AnalysisResult& analysis)
		{
			std::string output = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
			output += "<testsuite tests=\"" + std::to_string(analysis.findings.size()) + "\" failures=\"" +
				std::to_string(analysis.findings.size()) + "\" errors=\"0\" skipped=\"0\" name=\"CookScope\">\n";
			for (const Finding& finding : analysis.findings)
			{
				output += "  <testcase classname=\"" + EscapeXml(finding.ruleId) + "\" name=\"" + EscapeXml(finding.assetPath) + "\">\n";
				output += "    <failure message=\"" + EscapeXml(finding.message) + "\">" + EscapeXml(finding.message) + "</failure>\n";
				output += "  </testcase>\n";
			}
			output += "</testsuite>\n";
			return output;
		}

		std::string EscapeEmbeddedJson(std::string value)
		{
			std::string output;
			output.reserve(value.size());
			for (const char character : value)
			{
				if (character == '<') output += "\\u003c";
				else if (character == '&') output += "\\u0026";
				else output.push_back(character);
			}
			return output;
		}

		std::string RenderHtml(std::string_view canonicalJson)
		{
			std::string html = R"(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>CookScope report</title><style>
:root{color-scheme:dark;background:#0c111b;color:#e7edf7;font:14px system-ui,sans-serif}body{margin:0;padding:24px}main{max-width:1200px;margin:auto}h1{font-size:28px;margin:0 0 4px}.muted{color:#91a0b8}.controls{display:grid;grid-template-columns:repeat(5,minmax(120px,1fr));gap:8px;margin:20px 0}input,select{background:#151d2b;border:1px solid #344158;border-radius:7px;color:inherit;padding:9px}table{border-collapse:collapse;width:100%;background:#111827}th,td{border-bottom:1px solid #293449;padding:10px;text-align:left;vertical-align:top}th{color:#a9bad3}tr[data-severity=error]{border-left:3px solid #ff647c}tr[data-severity=warning]{border-left:3px solid #f7bd52}details{max-width:520px;white-space:pre-wrap}.pill{border:1px solid #40506a;border-radius:999px;padding:2px 7px}@media(max-width:760px){body{padding:12px}.controls{grid-template-columns:1fr 1fr}.table-wrap{overflow:auto}th,td{min-width:120px}}
</style></head><body><main><h1>CookScope</h1><div id="summary" class="muted"></div>
<section class="controls"><select id="severity-filter"><option value="">All severities</option><option>error</option><option>warning</option><option>note</option></select><select id="rule-filter"><option value="">All rules</option></select><select id="class-filter"><option value="">All classes</option></select><input id="path-search" placeholder="Search asset path"><select id="size-sort"><option value="path">Path order</option><option value="size-desc">Largest size first</option><option value="delta-desc">Largest delta first</option></select></section>
<div class="table-wrap"><table><thead><tr><th>Severity</th><th>Rule</th><th>Asset</th><th>Class</th><th>Message / dependency</th><th>Size</th></tr></thead><tbody id="findings"></tbody></table></div>
<script type="application/json" id="cookscope-data">__COOKSCOPE_DATA__</script><script>
'use strict';const data=JSON.parse(document.getElementById('cookscope-data').textContent);const assets=new Map(data.assets.map(a=>[a.objectPath,a]));const deltas=new Map(((data.diff&&data.diff.sizeChanges)||[]).map(d=>[d.assetPath,d.deltaBytes]));const controls=['severity-filter','rule-filter','class-filter','path-search','size-sort'].map(id=>document.getElementById(id));const esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const rules=[...new Set(data.findings.map(f=>f.ruleId))].sort(),classes=[...new Set(data.assets.map(a=>a.assetClass))].sort();rules.forEach(v=>document.getElementById('rule-filter').insertAdjacentHTML('beforeend',`<option>${esc(v)}</option>`));classes.forEach(v=>document.getElementById('class-filter').insertAdjacentHTML('beforeend',`<option>${esc(v)}</option>`));document.getElementById('summary').textContent=`${data.summary.assets} assets · ${data.summary.findings} findings · ${data.summary.errors} errors · ${data.summary.warnings} warnings`;
function render(){const sev=controls[0].value,rule=controls[1].value,cls=controls[2].value,q=controls[3].value.toLowerCase(),sort=controls[4].value;let rows=data.findings.filter(f=>(!sev||f.severity===sev)&&(!rule||f.ruleId===rule)&&(!cls||(assets.get(f.assetPath)||{}).assetClass===cls)&&(!q||f.assetPath.toLowerCase().includes(q)));rows.sort((a,b)=>sort==='size-desc'?((assets.get(b.assetPath)||{}).cookedBytes||0)-((assets.get(a.assetPath)||{}).cookedBytes||0):sort==='delta-desc'?(deltas.get(b.assetPath)||0)-(deltas.get(a.assetPath)||0):a.assetPath.localeCompare(b.assetPath));document.getElementById('findings').innerHTML=rows.map(f=>{const a=assets.get(f.assetPath)||{},path=(f.dependencyPath||[]).map(e=>`${e.source} --${e.kind}--> ${e.target}`).join('\n'),size=a.cookedBytes==null?'unavailable':`${a.cookedBytes} B`,delta=deltas.has(f.assetPath)?` (${deltas.get(f.assetPath)>=0?'+':''}${deltas.get(f.assetPath)} B)`:'';return `<tr data-severity="${esc(f.severity)}"><td><span class="pill">${esc(f.severity)}</span></td><td>${esc(f.ruleId)}</td><td>${esc(f.assetPath)}</td><td>${esc(a.assetClass||'')}</td><td>${esc(f.message)}${path?`<details><summary>Dependency path</summary>${esc(path)}</details>`:''}</td><td>${esc(size+delta)}</td></tr>`}).join('')};controls.forEach(c=>c.addEventListener(c.tagName==='INPUT'?'input':'change',render));render();
</script></main></body></html>
)";
			const std::string marker = "__COOKSCOPE_DATA__";
			html.replace(html.find(marker), marker.size(), EscapeEmbeddedJson(std::string(canonicalJson)));
			return html;
		}
	}

	ReportSet RenderReports(
		const Snapshot& snapshot,
		const RuleConfig& config,
		const AnalysisResult& analysis,
		const SnapshotDiffResult* diff)
	{
		ReportSet reports;
		reports.json = WriteCanonicalJson(CanonicalResult(snapshot, config, analysis, diff)) + "\n";
		reports.sarif = RenderSarif(config, analysis);
		reports.junit = RenderJUnit(analysis);
		reports.html = RenderHtml(reports.json);
		return reports;
	}
}
