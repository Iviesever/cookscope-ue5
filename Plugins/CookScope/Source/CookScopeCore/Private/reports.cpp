#include "cookscope/reports.h"

#include "cookscope/json.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
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
				item.object.emplace("packageName", ReportJsonString(asset.packageName));
				item.object.emplace("packagePath", ReportJsonString(asset.packagePath));
				item.object.emplace("assetClass", ReportJsonString(asset.assetClass));
				item.object.emplace("primaryAssetId", asset.primaryAssetId ? ReportJsonString(*asset.primaryAssetId) : JsonValue{});
				item.object.emplace("diskSizeKind", ReportJsonString(ReportMeasurementName(asset.diskSize.kind)));
				item.object.emplace("diskBytes", OptionalNumber(asset.diskSize.bytes));
				item.object.emplace("cookedSizeKind", ReportJsonString(ReportMeasurementName(asset.cookedSize.kind)));
				item.object.emplace("cookedBytes", OptionalNumber(asset.cookedSize.bytes));
				JsonValue chunks;
				chunks.type = JsonType::Array;
				for (const std::int32_t chunk : asset.chunkIds) chunks.array.push_back(ReportJsonNumber(static_cast<std::uint64_t>(chunk)));
				item.object.emplace("chunkIds", std::move(chunks));
				JsonValue bundles;
				bundles.type = JsonType::Array;
				for (const std::string& bundle : asset.assetBundles) bundles.array.push_back(ReportJsonString(bundle));
				item.object.emplace("assetBundles", std::move(bundles));
				JsonValue dependencies;
				dependencies.type = JsonType::Array;
				for (const DependencyEdge& dependency : asset.dependencies)
				{
					JsonValue edge;
					edge.type = JsonType::Object;
					edge.object.emplace("kind", ReportJsonString(ReportDependencyName(dependency.kind)));
					edge.object.emplace("target", ReportJsonString(dependency.target));
					dependencies.array.push_back(std::move(edge));
				}
				item.object.emplace("dependencies", std::move(dependencies));
				JsonValue tags;
				tags.type = JsonType::Object;
				for (const auto& [key, value] : asset.tags) tags.object.emplace(key, ReportJsonString(value));
				item.object.emplace("tags", std::move(tags));
				item.object.emplace("sourceProvenance", ReportJsonString(asset.sourceProvenance));
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
			for (const AnalysisDiagnostic& diagnostic : analysis.diagnostics)
			{
				JsonValue item;
				item.type = JsonType::Object;
				item.object.emplace("ruleId", ReportJsonString(diagnostic.ruleId));
				item.object.emplace("level", ReportJsonString("error"));
				JsonValue message;
				message.type = JsonType::Object;
				message.object.emplace("text", ReportJsonString("analysis diagnostic: " + diagnostic.message));
				item.object.emplace("message", std::move(message));
				JsonValue locations;
				locations.type = JsonType::Array;
				if (!diagnostic.assetPath.empty())
				{
					JsonValue location;
					location.type = JsonType::Object;
					JsonValue physical;
					physical.type = JsonType::Object;
					JsonValue artifact;
					artifact.type = JsonType::Object;
					artifact.object.emplace("uri", ReportJsonString(diagnostic.assetPath));
					physical.object.emplace("artifactLocation", std::move(artifact));
					location.object.emplace("physicalLocation", std::move(physical));
					locations.array.push_back(std::move(location));
				}
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

		std::string RenderJUnit(const RuleConfig& config, const AnalysisResult& analysis)
		{
			std::map<std::string, const RuleDefinition*, std::less<>> rules;
			std::set<std::string, std::less<>> testIds;
			for (const RuleDefinition& rule : config.rules)
			{
				rules.emplace(rule.id, &rule);
				testIds.insert(rule.id);
			}
			for (const Finding& finding : analysis.findings) testIds.insert(finding.ruleId);
			for (const AnalysisDiagnostic& diagnostic : analysis.diagnostics) testIds.insert(diagnostic.ruleId);

			auto Blocks = [&](const Finding& finding) {
				const auto rule = rules.find(finding.ruleId);
				const Severity threshold = rule == rules.end() ? Severity::Error : rule->second->failThreshold;
				return finding.severity >= threshold;
			};
			std::size_t failures = 0;
			std::size_t errors = 0;
			for (const std::string& id : testIds)
			{
				const bool hasDiagnostic = std::any_of(analysis.diagnostics.begin(), analysis.diagnostics.end(), [&](const AnalysisDiagnostic& diagnostic) {
					return diagnostic.ruleId == id;
				});
				if (hasDiagnostic)
				{
					++errors;
					continue;
				}
				if (std::any_of(analysis.findings.begin(), analysis.findings.end(), [&](const Finding& finding) {
					return finding.ruleId == id && Blocks(finding);
				})) ++failures;
			}

			std::string output = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
			output += "<testsuite tests=\"" + std::to_string(testIds.size()) + "\" failures=\"" +
				std::to_string(failures) + "\" errors=\"" + std::to_string(errors) + "\" skipped=\"0\" name=\"CookScope\">\n";
			for (const std::string& id : testIds)
			{
				output += "  <testcase classname=\"CookScope\" name=\"" + EscapeXml(id) + "\">\n";
				std::vector<const AnalysisDiagnostic*> diagnostics;
				std::vector<const Finding*> blocking;
				std::vector<const Finding*> nonBlocking;
				for (const AnalysisDiagnostic& diagnostic : analysis.diagnostics)
					if (diagnostic.ruleId == id) diagnostics.push_back(&diagnostic);
				for (const Finding& finding : analysis.findings)
				{
					if (finding.ruleId != id) continue;
					(Blocks(finding) ? blocking : nonBlocking).push_back(&finding);
				}
				if (!diagnostics.empty())
				{
					output += "    <error message=\"" + std::to_string(diagnostics.size()) + " diagnostics\">";
					for (const AnalysisDiagnostic* diagnostic : diagnostics)
						output += EscapeXml(diagnostic->assetPath + ": " + diagnostic->message) + "\n";
					output += "</error>\n";
				}
				else if (!blocking.empty())
				{
					output += "    <failure message=\"" + std::to_string(blocking.size()) + " blocking findings\">";
					for (const Finding* finding : blocking)
						output += EscapeXml(finding->assetPath + ": " + finding->message) + "\n";
					output += "</failure>\n";
				}
				if (!nonBlocking.empty())
				{
					output += "    <system-out>";
					for (const Finding* finding : nonBlocking)
						output += EscapeXml(finding->assetPath + ": " + finding->message) + "\n";
					output += "</system-out>\n";
				}
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
:root{color-scheme:dark;background:#0a0f18;color:#e8eef8;font:14px system-ui,sans-serif}*{box-sizing:border-box}body{margin:0;padding:24px}main{max-width:1320px;margin:auto}h1{font-size:30px;margin:0 0 4px}h2{font-size:18px;margin:28px 0 10px}.muted{color:#91a0b8}.cards{display:grid;grid-template-columns:repeat(4,1fr);gap:10px;margin:18px 0}.card,.comparison{background:#111827;border:1px solid #27344a;border-radius:9px;padding:12px}.card strong{display:block;font-size:20px;margin-top:3px}.comparison{line-height:1.55}.controls{display:grid;grid-template-columns:repeat(4,minmax(140px,1fr));gap:8px;margin:18px 0}input,select{width:100%;background:#151d2b;border:1px solid #344158;border-radius:7px;color:inherit;padding:9px}table{border-collapse:collapse;width:100%;background:#111827}th,td{border-bottom:1px solid #293449;padding:10px;text-align:left;vertical-align:top}th{color:#a9bad3;white-space:nowrap}tr[data-severity=error]{border-left:3px solid #ff647c}tr[data-severity=warning]{border-left:3px solid #f7bd52}details{max-width:620px;white-space:pre-wrap}.pill{border:1px solid #40506a;border-radius:999px;padding:2px 7px;white-space:nowrap}.empty{padding:18px;color:#91a0b8}.table-wrap{overflow:auto;border:1px solid #27344a;border-radius:9px}@media(max-width:760px){body{padding:12px}.cards,.controls{grid-template-columns:1fr 1fr}h1{font-size:25px}th,td{min-width:126px}.comparison{overflow-wrap:anywhere}}@media(max-width:430px){.cards{grid-template-columns:1fr 1fr}.controls{grid-template-columns:1fr}}
</style></head><body><main><h1>CookScope</h1><div class="muted">Offline asset dependency and Cook budget report</div>
<section class="cards"><div class="card"><span class="muted">Assets</span><strong id="asset-count">0</strong></div><div class="card"><span class="muted">Findings</span><strong id="finding-count">0</strong></div><div class="card"><span class="muted">Errors</span><strong id="error-count">0</strong></div><div class="card"><span class="muted">Warnings</span><strong id="warning-count">0</strong></div></section>
<section id="comparison-summary" class="comparison">No compatible baseline was supplied.</section>
<section class="controls"><select id="severity-filter"><option value="">All severities</option><option>error</option><option>warning</option><option>note</option></select><select id="rule-filter"><option value="">All rules</option></select><select id="class-filter"><option value="">All classes</option></select><input id="path-search" placeholder="Search asset path"><select id="chunk-filter"><option value="">All chunks</option></select><select id="bundle-filter"><option value="">All bundles</option></select><select id="change-filter"><option value="">All baseline states</option><option value="changed">Changed assets</option><option value="stable">Stable assets</option></select><select id="size-sort"><option value="path">Path order</option><option value="size-desc">Largest size first</option><option value="delta-desc">Largest delta first</option></select></section>
<h2>Findings</h2><div class="table-wrap"><table><thead><tr><th>Severity</th><th>Rule</th><th>Asset / aggregate</th><th>Class</th><th>Message / dependency</th><th>Measured size</th></tr></thead><tbody id="findings"></tbody></table></div>
<h2>Diagnostics</h2><div class="table-wrap"><table id="diagnostics-table"><thead><tr><th>Rule</th><th>Asset</th><th>Code</th><th>Message</th></tr></thead><tbody id="diagnostics"></tbody></table></div>
<h2>Assets, chunks, and bundles</h2><div class="table-wrap"><table id="asset-table"><thead><tr><th>Asset</th><th>Class</th><th>Cook size</th><th>Delta</th><th>Chunk</th><th>Bundle</th><th>Primary / dependencies</th></tr></thead><tbody id="assets"></tbody></table></div>
<script type="application/json" id="cookscope-data">__COOKSCOPE_DATA__</script><script>
'use strict';const data=JSON.parse(document.getElementById('cookscope-data').textContent),assetRows=Array.isArray(data.assets)?data.assets:[],findingRows=Array.isArray(data.findings)?data.findings:[],diagnosticRows=Array.isArray(data.diagnostics)?data.diagnostics:[],diff=data.diff&&data.diff.comparable?data.diff:null,assetMap=new Map(assetRows.map(a=>[a.objectPath,a])),deltas=new Map(((diff&&diff.sizeChanges)||[]).map(d=>[d.assetPath,d.deltaBytes])),changed=new Set();((diff&&diff.assetChanges)||[]).forEach(c=>{if(c.baselinePath)changed.add(c.baselinePath);if(c.candidatePath)changed.add(c.candidatePath)});((diff&&diff.sizeChanges)||[]).forEach(c=>changed.add(c.assetPath));const ids=['severity-filter','rule-filter','class-filter','path-search','chunk-filter','bundle-filter','change-filter','size-sort'],controls=Object.fromEntries(ids.map(id=>[id,document.getElementById(id)])),esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c])),option=(id,value)=>controls[id].insertAdjacentHTML('beforeend',`<option value="${esc(value)}">${esc(value)}</option>`);
document.getElementById('asset-count').textContent=data.summary.assets;document.getElementById('finding-count').textContent=data.summary.findings;document.getElementById('error-count').textContent=data.summary.errors;document.getElementById('warning-count').textContent=data.summary.warnings;[...new Set(findingRows.map(f=>f.ruleId))].sort().forEach(v=>option('rule-filter',v));[...new Set(assetRows.map(a=>a.assetClass))].sort().forEach(v=>option('class-filter',v));[...new Set(assetRows.flatMap(a=>a.chunkIds||[]))].sort((a,b)=>a-b).forEach(v=>option('chunk-filter',v));[...new Set(assetRows.flatMap(a=>a.assetBundles||[]))].sort().forEach(v=>option('bundle-filter',v));
if(diff){const summary=document.getElementById('comparison-summary'),assetChanges=diff.assetChanges||[],edgeChanges=diff.edgeChanges||[],sizeChanges=diff.sizeChanges||[],findingChanges=diff.findingChanges||[];summary.innerHTML=`<strong>Baseline / candidate</strong><div class="muted">${assetChanges.length} asset changes · ${edgeChanges.length} dependency changes · ${sizeChanges.length} size changes · ${findingChanges.length} finding changes</div>${assetChanges.length?`<details><summary>Asset changes</summary>${assetChanges.map(c=>esc(`${c.kind}: ${c.baselinePath||'—'} → ${c.candidatePath||'—'}${(c.fields||[]).length?` [${c.fields.join(', ')}]`:''}`)).join('\n')}</details>`:''}${edgeChanges.length?`<details><summary>Dependency changes</summary>${edgeChanges.map(c=>esc(`${c.kind}: ${c.source} → ${c.target}`)).join('\n')}</details>`:''}`}
function selected(){return{sev:controls['severity-filter'].value,rule:controls['rule-filter'].value,cls:controls['class-filter'].value,q:controls['path-search'].value.toLowerCase(),chunk:controls['chunk-filter'].value,bundle:controls['bundle-filter'].value,change:controls['change-filter'].value,sort:controls['size-sort'].value}}
function assetMatches(a,f){const isChanged=changed.has(a.objectPath);return(!f.cls||a.assetClass===f.cls)&&(!f.q||a.objectPath.toLowerCase().includes(f.q))&&(!f.chunk||(a.chunkIds||[]).map(String).includes(f.chunk))&&(!f.bundle||(a.assetBundles||[]).includes(f.bundle))&&(!f.change||(f.change==='changed'?isChanged:!isChanged))}
const findingMatches=(x,f)=>{const a=assetMap.get(x.assetPath);if(a)return assetMatches(a,f);const baselineChanged=x.baselineState==='new'||x.baselineState==='worsened';return(!f.cls||x.assetPath===f.cls)&&(!f.q||x.assetPath.toLowerCase().includes(f.q))&&!f.chunk&&!f.bundle&&(!f.change||(f.change==='changed'?baselineChanged:!baselineChanged))};
function sortAssets(rows,mode){rows.sort((a,b)=>mode==='size-desc'?(b.cookedBytes||0)-(a.cookedBytes||0):mode==='delta-desc'?(deltas.get(b.objectPath)||0)-(deltas.get(a.objectPath)||0):a.objectPath.localeCompare(b.objectPath));return rows}
function render(){const f=selected(),visibleAssets=sortAssets(assetRows.filter(a=>assetMatches(a,f)),f.sort);let findings=findingRows.filter(x=>(!f.sev||x.severity===f.sev)&&(!f.rule||x.ruleId===f.rule)&&findingMatches(x,f));findings.sort((a,b)=>f.sort==='size-desc'?((assetMap.get(b.assetPath)||{}).cookedBytes||b.observedBytes||0)-((assetMap.get(a.assetPath)||{}).cookedBytes||a.observedBytes||0):f.sort==='delta-desc'?(deltas.get(b.assetPath)||0)-(deltas.get(a.assetPath)||0):a.assetPath.localeCompare(b.assetPath));document.getElementById('findings').innerHTML=findings.length?findings.map(x=>{const a=assetMap.get(x.assetPath)||{},path=(x.dependencyPath||[]).map(e=>`${e.source} --${e.kind}--> ${e.target}`).join('\n'),measured=a.cookedBytes??x.observedBytes,size=measured==null?'unavailable':`${measured} B`,budget=x.budgetBytes==null?'':` / ${x.budgetBytes} B budget`;return `<tr data-severity="${esc(x.severity)}"><td><span class="pill">${esc(x.severity)}</span></td><td>${esc(x.ruleId)}<div class="muted">${esc(x.baselineState)}</div></td><td>${esc(x.assetPath)}</td><td>${esc(a.assetClass||'Aggregate')}</td><td>${esc(x.message)}${path?`<details><summary>Full dependency chain</summary>${esc(path)}</details>`:''}</td><td>${esc(size+budget)}</td></tr>`}).join(''):`<tr><td colspan="6" class="empty">No findings match the current filters.</td></tr>`;const diagnostics=diagnosticRows.filter(x=>(!f.rule||x.ruleId===f.rule)&&(!f.q||x.assetPath.toLowerCase().includes(f.q)));document.getElementById('diagnostics').innerHTML=diagnostics.length?diagnostics.map(x=>`<tr><td>${esc(x.ruleId)}</td><td>${esc(x.assetPath||'—')}</td><td>${esc(x.code)}</td><td>${esc(x.message)}</td></tr>`).join(''):`<tr><td colspan="4" class="empty">No analysis diagnostics.</td></tr>`;document.getElementById('assets').innerHTML=visibleAssets.length?visibleAssets.map(a=>{const dependency=(a.dependencies||[]).map(e=>`${e.kind} → ${e.target}`).join('\n'),delta=deltas.get(a.objectPath),size=a.cookedBytes==null?'unavailable':`${a.cookedBytes} B`;return `<tr><td>${esc(a.objectPath)}</td><td>${esc(a.assetClass)}</td><td>${esc(size)}</td><td>${delta==null?'—':esc(`${delta>=0?'+':''}${delta} B`)}</td><td>${esc((a.chunkIds||[]).join(', ')||'—')}</td><td>${esc((a.assetBundles||[]).join(', ')||'—')}</td><td>${esc(a.primaryAssetId||'—')}${dependency?`<details><summary>${a.dependencies.length} dependencies</summary>${esc(dependency)}</details>`:''}</td></tr>`}).join(''):`<tr><td colspan="7" class="empty">No assets match the current filters.</td></tr>`}
Object.values(controls).forEach(c=>c.addEventListener(c.tagName==='INPUT'?'input':'change',render));render();
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
		reports.junit = RenderJUnit(config, analysis);
		reports.html = RenderHtml(reports.json);
		return reports;
	}
}
