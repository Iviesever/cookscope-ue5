#include "cookscope/snapshot.h"

#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace
{
	int Fail(std::string_view message)
	{
		std::cerr << "FAIL: " << message << '\n';
		return 1;
	}

	std::string Asset(std::string_view objectPath, std::string_view dependencies, std::string_view cookedSize)
	{
		const std::size_t dot = objectPath.find('.');
		const std::string package(objectPath.substr(0, dot));
		const std::size_t slash = package.find_last_of('/');
		const std::string packagePath = package.substr(0, slash);
		return "{\"objectPath\":\"" + std::string(objectPath) +
			"\",\"packageName\":\"" + package +
			"\",\"assetClass\":\"Texture2D\",\"packagePath\":\"" + packagePath +
			"\",\"primaryAssetId\":null,\"diskSize\":{\"kind\":\"package-disk\",\"bytes\":128},"
			"\"cookedSize\":" + std::string(cookedSize) +
			",\"chunkIds\":[2,0],\"assetBundles\":[\"UI\",\"Default\"],"
			"\"tags\":{\"用途\":\"测试\",\"FixtureId\":\"asset\"},\"dependencies\":[" +
			std::string(dependencies) + "],\"sourceProvenance\":\"asset-registry\"}";
	}

	std::string Snapshot(std::string assets, std::string_view extra = {})
	{
		return "{\"schema\":\"cookscope.snapshot/1\",\"provenance\":{"
			"\"engineVersion\":\"5.8.0-55116800\",\"platform\":\"Windows\","
			"\"cookConfiguration\":\"Development\","
			"\"sourceSha\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"},"
			"\"assets\":[" + std::move(assets) + "]" + std::string(extra) + "}";
	}
}

int main()
{
	using cookscope::DependencyKind;
	using cookscope::MeasurementKind;
	using cookscope::SnapshotErrorCode;

	const std::string allEdges =
		"{\"target\":\"/Game/Z.Z\",\"kind\":\"soft\"},"
		"{\"target\":\"/Game/A.A\",\"kind\":\"hard\"},"
		"{\"target\":\"/Game/M.M\",\"kind\":\"manage\"},"
		"{\"target\":\"/Game/S.S\",\"kind\":\"searchable-name\"}";
	const std::string noEdges;
	const std::string cooked = "{\"kind\":\"actual-cooked\",\"bytes\":96}";
	const std::string unavailable = "{\"kind\":\"unavailable\"}";
	const std::string assetB = Asset("/Game/测试/B.B", allEdges, cooked);
	const std::string assetA = Asset("/Game/测试/A.A", noEdges, unavailable);

	const auto first = cookscope::ParseSnapshot(Snapshot(assetB + "," + assetA));
	const auto second = cookscope::ParseSnapshot(Snapshot(assetA + "," + assetB));
	if (!first.ok || !second.ok)
	{
		return Fail("valid complete snapshots must parse");
	}
	if (first.value.assets[1].dependencies.size() != 4 ||
		first.value.assets[1].dependencies[0].kind != DependencyKind::Hard ||
		first.value.assets[1].dependencies[1].kind != DependencyKind::Soft ||
		first.value.assets[1].dependencies[2].kind != DependencyKind::Manage ||
		first.value.assets[1].dependencies[3].kind != DependencyKind::SearchableName)
	{
		return Fail("all four dependency kinds must remain distinct and canonical");
	}
	if (first.value.assets[1].cookedSize.kind != MeasurementKind::ActualCooked ||
		first.value.assets[0].cookedSize.kind != MeasurementKind::Unavailable)
	{
		return Fail("actual cooked and unavailable measurements must remain distinct");
	}

	const std::string firstCanonical = cookscope::WriteCanonicalSnapshot(first.value);
	const std::string secondCanonical = cookscope::WriteCanonicalSnapshot(second.value);
	if (firstCanonical != secondCanonical ||
		firstCanonical.find("/Game/测试/A.A") > firstCanonical.find("/Game/测试/B.B") ||
		firstCanonical.back() != '\n')
	{
		return Fail("snapshot canonical bytes must sort assets and end with LF");
	}

	std::string duplicateFacts = assetB;
	duplicateFacts.replace(
		duplicateFacts.find("[\"UI\",\"Default\"]"),
		std::string("[\"UI\",\"Default\"]").size(),
		"[\"UI\",\"Default\",\"Default\"]");
	const std::string duplicateEdge = "{\"target\":\"/Game/A.A\",\"kind\":\"hard\"},";
	duplicateFacts.insert(duplicateFacts.find("\"dependencies\":[") + std::string("\"dependencies\":[").size(), duplicateEdge);
	const auto normalizedFacts = cookscope::ParseSnapshot(Snapshot(duplicateFacts));
	if (!normalizedFacts.ok || normalizedFacts.value.assets[0].assetBundles.size() != 2 ||
		normalizedFacts.value.assets[0].dependencies.size() != 4)
	{
		return Fail("duplicate Bundle and typed-edge facts must normalize to one semantic value");
	}

	const auto duplicateAsset = cookscope::ParseSnapshot(Snapshot(assetA + "," + assetA));
	if (duplicateAsset.ok || duplicateAsset.error.code != SnapshotErrorCode::DuplicateAsset ||
		duplicateAsset.error.path != "$.assets[1].objectPath")
	{
		return Fail("duplicate object paths must fail closed at a stable path");
	}

	const auto missingBytes = cookscope::ParseSnapshot(Snapshot(Asset(
		"/Game/Bad/Missing.Missing", noEdges, "{\"kind\":\"actual-cooked\"}")));
	if (missingBytes.ok || missingBytes.error.code != SnapshotErrorCode::MissingField)
	{
		return Fail("available measurements must require bytes");
	}

	const auto unavailableBytes = cookscope::ParseSnapshot(Snapshot(Asset(
		"/Game/Bad/Unavailable.Unavailable", noEdges, "{\"kind\":\"unavailable\",\"bytes\":1}")));
	if (unavailableBytes.ok || unavailableBytes.error.code != SnapshotErrorCode::InvalidValue)
	{
		return Fail("unavailable measurements must reject fabricated bytes");
	}

	std::string unknownAsset = assetA;
	unknownAsset.insert(unknownAsset.size() - 1, ",\"surprise\":true");
	const auto unknownAssetResult = cookscope::ParseSnapshot(Snapshot(unknownAsset));
	if (unknownAssetResult.ok || unknownAssetResult.error.code != SnapshotErrorCode::UnknownField ||
		unknownAssetResult.error.path != "$.assets[0].surprise")
	{
		return Fail("unknown asset fields must fail closed");
	}

	const auto unknownRoot = cookscope::ParseSnapshot(Snapshot(assetA, ",\"unexpected\":true"));
	if (unknownRoot.ok || unknownRoot.error.code != SnapshotErrorCode::UnknownField ||
		unknownRoot.error.path != "$.unexpected")
	{
		return Fail("unknown snapshot fields must fail closed");
	}

	std::cout << "PASS: strict deterministic Asset Snapshot contract\n";
	return 0;
}
