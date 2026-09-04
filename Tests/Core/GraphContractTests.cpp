#include "cookscope/graph.h"

#include <iostream>
#include <string>
#include <string_view>

namespace
{
	int Fail(std::string_view message)
	{
		std::cerr << "FAIL: " << message << '\n';
		return 1;
	}

	cookscope::AssetRecord Asset(std::string objectPath, std::vector<cookscope::DependencyEdge> edges = {})
	{
		cookscope::AssetRecord result;
		result.objectPath = std::move(objectPath);
		result.dependencies = std::move(edges);
		return result;
	}
}

int main()
{
	using cookscope::DependencyKind;
	using cookscope::GraphErrorCode;
	using cookscope::OperationState;

	cookscope::Snapshot snapshot;
	snapshot.assets = {
		Asset("/Game/E.E"),
		Asset("/Game/C.C", {{"/Game/D.D", DependencyKind::SearchableName}}),
		Asset("/Game/A.A", {{"/Game/C.C", DependencyKind::Soft}, {"/Game/B.B", DependencyKind::Hard}}),
		Asset("/Game/D.D", {{"/Game/A.A", DependencyKind::Hard}}),
		Asset("/Game/B.B", {{"/Game/D.D", DependencyKind::Manage}}),
	};

	const auto built = cookscope::BuildDependencyGraph(snapshot, cookscope::OperationLimits{});
	if (built.state != OperationState::Complete || built.graph.nodes.size() != 5 ||
		built.graph.nodes.front() != "/Game/A.A" || built.graph.nodes.back() != "/Game/E.E")
	{
		return Fail("graph build must produce stable sorted nodes");
	}

	const auto direct = cookscope::DirectDependencies(built.graph, "/Game/A.A", cookscope::DependencyMask::All());
	if (direct.size() != 2 || direct[0].target != "/Game/B.B" || direct[0].kind != DependencyKind::Hard ||
		direct[1].target != "/Game/C.C" || direct[1].kind != DependencyKind::Soft)
	{
		return Fail("direct dependencies must preserve typed stable order");
	}
	const auto hardOnly = cookscope::DirectDependencies(built.graph, "/Game/A.A", cookscope::DependencyMask::Only(DependencyKind::Hard));
	if (hardOnly.size() != 1 || hardOnly[0].target != "/Game/B.B")
	{
		return Fail("edge masks must not collapse dependency kinds");
	}
	const auto hardOnlyPath = cookscope::FindShortestPath(
		built.graph, "/Game/A.A", "/Game/D.D", cookscope::DependencyMask::Only(DependencyKind::Hard), cookscope::OperationLimits{});
	if (hardOnlyPath.state != OperationState::Complete || hardOnlyPath.found)
	{
		return Fail("typed traversal must not cross a filtered Manage edge");
	}

	const auto referencers = cookscope::Referencers(built.graph, "/Game/D.D", cookscope::DependencyMask::All());
	if (referencers.size() != 2 || referencers[0].source != "/Game/B.B" ||
		referencers[1].source != "/Game/C.C")
	{
		return Fail("reverse references must be stable and typed");
	}

	const auto shortest = cookscope::FindShortestPath(
		built.graph, "/Game/A.A", "/Game/D.D", cookscope::DependencyMask::All(), cookscope::OperationLimits{});
	if (shortest.state != OperationState::Complete || !shortest.found || shortest.steps.size() != 2 ||
		shortest.steps[0].source != "/Game/A.A" || shortest.steps[0].target != "/Game/B.B" ||
		shortest.steps[1].kind != DependencyKind::Manage)
	{
		return Fail("shortest path must select the stable typed path");
	}

	cookscope::OperationLimits onePath;
	onePath.maximumPaths = 1;
	const auto paths = cookscope::FindAllPaths(
		built.graph, "/Game/A.A", "/Game/D.D", cookscope::DependencyMask::All(), onePath);
	if (paths.state != OperationState::Truncated || paths.paths.size() != 1 || paths.paths[0] != shortest.steps)
	{
		return Fail("all-path traversal must report explicit truncation at the path limit");
	}

	const auto cycles = cookscope::FindCycles(built.graph, cookscope::DependencyMask::All(), cookscope::OperationLimits{});
	if (cycles.state != OperationState::Complete || cycles.cycles.size() != 1 ||
		cycles.cycles[0].nodes != std::vector<std::string>({"/Game/A.A", "/Game/B.B", "/Game/C.C", "/Game/D.D"}))
	{
		return Fail("cycle detection must return stable strongly connected components");
	}

	cookscope::OperationLimits shallow;
	shallow.maximumDepth = 1;
	const auto depthLimited = cookscope::FindShortestPath(
		built.graph, "/Game/A.A", "/Game/D.D", cookscope::DependencyMask::All(), shallow);
	if (depthLimited.state != OperationState::Truncated || depthLimited.found)
	{
		return Fail("depth-limited traversal must fail closed with explicit truncation");
	}

	cookscope::OperationLimits tiny;
	tiny.maximumNodes = 3;
	const auto rejected = cookscope::BuildDependencyGraph(snapshot, tiny);
	if (rejected.state != OperationState::Failed || rejected.error.code != GraphErrorCode::NodeLimitExceeded)
	{
		return Fail("graph build must fail closed when the node limit is exceeded");
	}

	cookscope::OperationLimits tinyEdges;
	tinyEdges.maximumEdges = 2;
	const auto edgeRejected = cookscope::BuildDependencyGraph(snapshot, tinyEdges);
	if (edgeRejected.state != OperationState::Failed || edgeRejected.error.code != GraphErrorCode::EdgeLimitExceeded)
	{
		return Fail("graph build must fail closed when the edge limit is exceeded");
	}

	cookscope::Snapshot unresolvedSnapshot = snapshot;
	unresolvedSnapshot.assets[2].dependencies.push_back({"/Game/Missing.Missing", DependencyKind::Soft});
	const auto unresolved = cookscope::BuildDependencyGraph(unresolvedSnapshot, cookscope::OperationLimits{});
	if (unresolved.state != OperationState::Complete ||
		unresolved.graph.unresolvedTargets != std::vector<std::string>({"/Game/Missing.Missing"}))
	{
		return Fail("unresolved dependency targets must be explicit and stable");
	}

	std::cout << "PASS: deterministic typed dependency graph contract\n";
	return 0;
}
