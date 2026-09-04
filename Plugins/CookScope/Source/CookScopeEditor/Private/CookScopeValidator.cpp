#include "CookScopeValidator.h"

#include "CookScopeAssetScanner.h"

#include "cookscope/rule_config.h"
#include "cookscope/rules.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include <string>

namespace
{
	std::string ToUtf8(const FString& Text)
	{
		FTCHARToUTF8 Converted(*Text);
		return std::string(Converted.Get(), static_cast<std::size_t>(Converted.Length()));
	}

	bool LoadCookScopeRules(cookscope::RuleConfig& Output, FString& Error)
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CookScope"));
		if (!Plugin)
		{
			Error = TEXT("CookScope plugin metadata is unavailable");
			return false;
		}
		const FString ConfigPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Config"), TEXT("CookScopeRules.json"));
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *ConfigPath))
		{
			Error = FString::Printf(TEXT("Unable to read CookScope rules: %s"), *ConfigPath);
			return false;
		}
		const cookscope::RuleConfigParseResult Parsed = cookscope::ParseRuleConfig(ToUtf8(Text));
		if (!Parsed.ok)
		{
			Error = FString::Printf(
				TEXT("CookScope rules invalid at %s: %s"),
				UTF8_TO_TCHAR(Parsed.error.path.c_str()),
				UTF8_TO_TCHAR(Parsed.error.message.c_str()));
			return false;
		}
		Output = Parsed.value;
		return true;
	}
}

bool UCookScopeValidator::CanValidateAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InObject,
	FDataValidationContext& InContext) const
{
	(void)InContext;
	return InObject != nullptr && InAssetData.PackageName.ToString().StartsWith(TEXT("/Game/"));
}

EDataValidationResult UCookScopeValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& Context)
{
	(void)Context;
	cookscope::RuleConfig Rules;
	FString Error;
	if (!LoadCookScopeRules(Rules, Error))
	{
		AssetFails(InAsset, FText::FromString(Error));
		return EDataValidationResult::Invalid;
	}

	const FCookScopeScanResult Scan = FCookScopeAssetScanner::ScanPath(
		InAssetData.PackagePath.ToString(),
		TEXT("0000000000000000000000000000000000000000"));
	if (!Scan.bSuccess)
	{
		AssetFails(InAsset, FText::FromString(Scan.Error));
		return EDataValidationResult::Invalid;
	}

	const std::string ObjectPath = ToUtf8(InAssetData.GetSoftObjectPath().ToString());
	const cookscope::AnalysisResult Analysis = cookscope::Evaluate(Scan.Snapshot, Rules);
	bool failed = false;
	bool warned = false;
	for (const cookscope::AnalysisDiagnostic& Diagnostic : Analysis.diagnostics)
	{
		if (!Diagnostic.assetPath.empty() && Diagnostic.assetPath != ObjectPath) continue;
		AssetFails(InAsset, FText::FromString(FString::Printf(
			TEXT("[%s] %s"),
			UTF8_TO_TCHAR(Diagnostic.ruleId.c_str()),
			UTF8_TO_TCHAR(Diagnostic.message.c_str()))));
		failed = true;
	}
	for (const cookscope::Finding& Finding : Analysis.findings)
	{
		if (Finding.assetPath != ObjectPath) continue;
		const FText Message = FText::FromString(FString::Printf(
			TEXT("[%s] %s"),
			UTF8_TO_TCHAR(Finding.ruleId.c_str()),
			UTF8_TO_TCHAR(Finding.message.c_str())));
		if (Finding.severity == cookscope::Severity::Error)
		{
			AssetFails(InAsset, Message);
			failed = true;
		}
		else
		{
			AssetWarning(InAsset, Message);
			warned = true;
		}
	}

	if (!failed && !warned)
	{
		AssetPasses(InAsset);
	}
	return failed ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

