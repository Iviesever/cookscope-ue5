#pragma once

#include "cookscope/snapshot.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace cookscope
{
	class COOKSCOPECORE_API DependencyMask
	{
	public:
		static DependencyMask All() noexcept;
		static DependencyMask Only(DependencyKind kind) noexcept;
		[[nodiscard]] bool Contains(DependencyKind kind) const noexcept;

	private:
		explicit DependencyMask(std::uint8_t bits) noexcept : Bits(bits) {}
		std::uint8_t Bits = 0;
	};

	struct OperationLimits
	{
		std::size_t maximumNodes = 100000;
		std::size_t maximumEdges = 500000;
		std::size_t maximumDepth = 64;
		std::size_t maximumPaths = 1024;
	};

	enum class OperationState : std::uint8_t
	{
		Complete,
		Truncated,
		Failed,
	};

	enum class GraphErrorCode : std::uint8_t
	{
		None,
		NodeLimitExceeded,
		EdgeLimitExceeded,
		UnknownNode,
	};

	struct GraphError
	{
		GraphErrorCode code = GraphErrorCode::None;
		std::string message;
	};

	struct GraphArc
	{
		std::size_t node = 0;
		DependencyKind kind = DependencyKind::Hard;
		bool operator==(const GraphArc&) const = default;
	};

	struct DependencyGraph
	{
		std::vector<std::string> nodes;
		std::map<std::string, std::size_t, std::less<>> nodeIndices;
		std::vector<std::vector<GraphArc>> outgoing;
		std::vector<std::vector<GraphArc>> incoming;
		std::vector<std::string> unresolvedTargets;
	};

	struct GraphBuildResult
	{
		OperationState state = OperationState::Failed;
		DependencyGraph graph;
		GraphError error;
	};

	struct DependencyStep
	{
		std::string source;
		std::string target;
		DependencyKind kind = DependencyKind::Hard;
		bool operator==(const DependencyStep&) const = default;
	};

	struct PathResult
	{
		OperationState state = OperationState::Complete;
		bool found = false;
		std::vector<DependencyStep> steps;
		std::size_t visitedNodes = 0;
		std::size_t traversedEdges = 0;
		GraphError error;
	};

	struct PathsResult
	{
		OperationState state = OperationState::Complete;
		std::vector<std::vector<DependencyStep>> paths;
		std::size_t visitedNodes = 0;
		std::size_t traversedEdges = 0;
		GraphError error;
	};

	struct WhyCookedResult
	{
		OperationState state = OperationState::Complete;
		bool found = false;
		std::string root;
		std::vector<DependencyStep> steps;
		std::size_t visitedNodes = 0;
		std::size_t traversedEdges = 0;
		GraphError error;
	};

	struct Cycle
	{
		std::vector<std::string> nodes;
	};

	struct CyclesResult
	{
		OperationState state = OperationState::Complete;
		std::vector<Cycle> cycles;
		std::size_t visitedNodes = 0;
		std::size_t traversedEdges = 0;
		GraphError error;
	};

	[[nodiscard]] COOKSCOPECORE_API GraphBuildResult BuildDependencyGraph(
		const Snapshot& snapshot,
		OperationLimits limits);
	[[nodiscard]] COOKSCOPECORE_API std::vector<DependencyStep> DirectDependencies(
		const DependencyGraph& graph,
		std::string_view source,
		DependencyMask mask);
	[[nodiscard]] COOKSCOPECORE_API std::vector<DependencyStep> Referencers(
		const DependencyGraph& graph,
		std::string_view target,
		DependencyMask mask);
	[[nodiscard]] COOKSCOPECORE_API PathResult FindShortestPath(
		const DependencyGraph& graph,
		std::string_view source,
		std::string_view target,
		DependencyMask mask,
		OperationLimits limits);
	[[nodiscard]] COOKSCOPECORE_API PathsResult FindAllPaths(
		const DependencyGraph& graph,
		std::string_view source,
		std::string_view target,
		DependencyMask mask,
		OperationLimits limits);
	[[nodiscard]] COOKSCOPECORE_API WhyCookedResult ExplainWhyCooked(
		const DependencyGraph& graph,
		std::span<const std::string_view> roots,
		std::string_view target,
		DependencyMask mask,
		OperationLimits limits);
	[[nodiscard]] COOKSCOPECORE_API CyclesResult FindCycles(
		const DependencyGraph& graph,
		DependencyMask mask,
		OperationLimits limits);
}
