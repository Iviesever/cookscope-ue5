#include "cookscope/graph.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <set>
#include <utility>

namespace cookscope
{
	namespace
	{
		std::uint8_t Bit(DependencyKind kind) noexcept
		{
			return static_cast<std::uint8_t>(1u << static_cast<std::uint8_t>(kind));
		}

		bool ArcLess(const GraphArc& left, const GraphArc& right, const DependencyGraph& graph)
		{
			return left.kind < right.kind || (left.kind == right.kind && graph.nodes[left.node] < graph.nodes[right.node]);
		}

		bool StepLess(const DependencyStep& left, const DependencyStep& right)
		{
			return left.source < right.source ||
				(left.source == right.source && (left.kind < right.kind ||
					(left.kind == right.kind && left.target < right.target)));
		}

		std::optional<std::size_t> FindNode(const DependencyGraph& graph, std::string_view name)
		{
			const auto found = graph.nodeIndices.find(name);
			if (found == graph.nodeIndices.end()) return std::nullopt;
			return found->second;
		}
	}

	DependencyMask DependencyMask::All() noexcept
	{
		return DependencyMask(Bit(DependencyKind::Hard) | Bit(DependencyKind::Soft) |
			Bit(DependencyKind::Manage) | Bit(DependencyKind::SearchableName));
	}

	DependencyMask DependencyMask::Only(DependencyKind kind) noexcept
	{
		return DependencyMask(Bit(kind));
	}

	bool DependencyMask::Contains(DependencyKind kind) const noexcept
	{
		return (Bits & Bit(kind)) != 0;
	}

	GraphBuildResult BuildDependencyGraph(const Snapshot& snapshot, OperationLimits limits)
	{
		GraphBuildResult result;
		std::set<std::string, std::less<>> knownAssets;
		std::set<std::string, std::less<>> nodeNames;
		for (const AssetRecord& asset : snapshot.assets)
		{
			knownAssets.insert(asset.objectPath);
			nodeNames.insert(asset.objectPath);
			for (const DependencyEdge& edge : asset.dependencies) nodeNames.insert(edge.target);
		}
		if (nodeNames.size() > limits.maximumNodes)
		{
			result.error = {GraphErrorCode::NodeLimitExceeded, "graph node limit exceeded"};
			return result;
		}

		result.graph.nodes.assign(nodeNames.begin(), nodeNames.end());
		result.graph.outgoing.resize(result.graph.nodes.size());
		result.graph.incoming.resize(result.graph.nodes.size());
		for (std::size_t index = 0; index < result.graph.nodes.size(); ++index)
		{
			result.graph.nodeIndices.emplace(result.graph.nodes[index], index);
			if (!knownAssets.contains(result.graph.nodes[index])) result.graph.unresolvedTargets.push_back(result.graph.nodes[index]);
		}

		std::size_t edgeCount = 0;
		for (const AssetRecord& asset : snapshot.assets)
		{
			const std::size_t source = result.graph.nodeIndices.at(asset.objectPath);
			for (const DependencyEdge& edge : asset.dependencies)
			{
				const std::size_t target = result.graph.nodeIndices.at(edge.target);
				result.graph.outgoing[source].push_back({target, edge.kind});
			}
			auto& outgoing = result.graph.outgoing[source];
			std::sort(outgoing.begin(), outgoing.end(), [&](const GraphArc& left, const GraphArc& right) {
				return ArcLess(left, right, result.graph);
			});
			outgoing.erase(std::unique(outgoing.begin(), outgoing.end()), outgoing.end());
			edgeCount += outgoing.size();
			if (edgeCount > limits.maximumEdges)
			{
				result.graph = {};
				result.error = {GraphErrorCode::EdgeLimitExceeded, "graph edge limit exceeded"};
				return result;
			}
		}

		for (std::size_t source = 0; source < result.graph.outgoing.size(); ++source)
		{
			for (const GraphArc& edge : result.graph.outgoing[source])
			{
				result.graph.incoming[edge.node].push_back({source, edge.kind});
			}
		}
		for (auto& incoming : result.graph.incoming)
		{
			std::sort(incoming.begin(), incoming.end(), [&](const GraphArc& left, const GraphArc& right) {
				return ArcLess(left, right, result.graph);
			});
		}
		result.state = OperationState::Complete;
		return result;
	}

	std::vector<DependencyStep> DirectDependencies(
		const DependencyGraph& graph,
		std::string_view source,
		DependencyMask mask)
	{
		std::vector<DependencyStep> result;
		const auto sourceIndex = FindNode(graph, source);
		if (!sourceIndex) return result;
		for (const GraphArc& edge : graph.outgoing[*sourceIndex])
		{
			if (mask.Contains(edge.kind)) result.push_back({std::string(source), graph.nodes[edge.node], edge.kind});
		}
		return result;
	}

	std::vector<DependencyStep> Referencers(
		const DependencyGraph& graph,
		std::string_view target,
		DependencyMask mask)
	{
		std::vector<DependencyStep> result;
		const auto targetIndex = FindNode(graph, target);
		if (!targetIndex) return result;
		for (const GraphArc& edge : graph.incoming[*targetIndex])
		{
			if (mask.Contains(edge.kind)) result.push_back({graph.nodes[edge.node], std::string(target), edge.kind});
		}
		std::sort(result.begin(), result.end(), StepLess);
		return result;
	}

	PathResult FindShortestPath(
		const DependencyGraph& graph,
		std::string_view source,
		std::string_view target,
		DependencyMask mask,
		OperationLimits limits)
	{
		PathResult result;
		const auto sourceIndex = FindNode(graph, source);
		const auto targetIndex = FindNode(graph, target);
		if (!sourceIndex || !targetIndex)
		{
			result.state = OperationState::Failed;
			result.error = {GraphErrorCode::UnknownNode, "source or target node is not present"};
			return result;
		}
		if (*sourceIndex == *targetIndex)
		{
			result.found = true;
			result.visitedNodes = 1;
			return result;
		}

		struct Previous
		{
			std::size_t node = 0;
			DependencyKind kind = DependencyKind::Hard;
		};
		std::vector<std::optional<Previous>> previous(graph.nodes.size());
		std::vector<std::size_t> depth(graph.nodes.size(), std::numeric_limits<std::size_t>::max());
		std::queue<std::size_t> queue;
		queue.push(*sourceIndex);
		depth[*sourceIndex] = 0;
		result.visitedNodes = 1;
		bool truncated = limits.maximumNodes == 0;

		while (!queue.empty() && !result.found)
		{
			const std::size_t current = queue.front();
			queue.pop();
			if (depth[current] >= limits.maximumDepth)
			{
				for (const GraphArc& edge : graph.outgoing[current]) if (mask.Contains(edge.kind)) truncated = true;
				continue;
			}
			for (const GraphArc& edge : graph.outgoing[current])
			{
				if (!mask.Contains(edge.kind)) continue;
				if (result.traversedEdges >= limits.maximumEdges)
				{
					truncated = true;
					break;
				}
				++result.traversedEdges;
				if (depth[edge.node] != std::numeric_limits<std::size_t>::max()) continue;
				if (result.visitedNodes >= limits.maximumNodes)
				{
					truncated = true;
					continue;
				}
				++result.visitedNodes;
				depth[edge.node] = depth[current] + 1;
				previous[edge.node] = Previous{current, edge.kind};
				if (edge.node == *targetIndex)
				{
					result.found = true;
					break;
				}
				queue.push(edge.node);
			}
		}

		if (result.found)
		{
			std::size_t current = *targetIndex;
			while (current != *sourceIndex)
			{
				const Previous link = *previous[current];
				result.steps.push_back({graph.nodes[link.node], graph.nodes[current], link.kind});
				current = link.node;
			}
			std::reverse(result.steps.begin(), result.steps.end());
		}
		result.state = truncated ? OperationState::Truncated : OperationState::Complete;
		return result;
	}

	PathsResult FindAllPaths(
		const DependencyGraph& graph,
		std::string_view source,
		std::string_view target,
		DependencyMask mask,
		OperationLimits limits)
	{
		PathsResult result;
		const auto sourceIndex = FindNode(graph, source);
		const auto targetIndex = FindNode(graph, target);
		if (!sourceIndex || !targetIndex)
		{
			result.state = OperationState::Failed;
			result.error = {GraphErrorCode::UnknownNode, "source or target node is not present"};
			return result;
		}

		std::vector<bool> inPath(graph.nodes.size(), false);
		std::vector<DependencyStep> currentPath;
		bool truncated = limits.maximumPaths == 0 || limits.maximumNodes == 0;
		std::function<void(std::size_t, std::size_t)> visit = [&](std::size_t node, std::size_t depth) {
			if (truncated && result.paths.size() >= limits.maximumPaths) return;
			if (result.visitedNodes >= limits.maximumNodes)
			{
				truncated = true;
				return;
			}
			++result.visitedNodes;
			if (node == *targetIndex)
			{
				result.paths.push_back(currentPath);
				if (result.paths.size() >= limits.maximumPaths) truncated = true;
				return;
			}
			if (depth >= limits.maximumDepth)
			{
				for (const GraphArc& edge : graph.outgoing[node]) if (mask.Contains(edge.kind)) truncated = true;
				return;
			}

			inPath[node] = true;
			for (const GraphArc& edge : graph.outgoing[node])
			{
				if (!mask.Contains(edge.kind) || inPath[edge.node]) continue;
				if (result.traversedEdges >= limits.maximumEdges)
				{
					truncated = true;
					break;
				}
				++result.traversedEdges;
				currentPath.push_back({graph.nodes[node], graph.nodes[edge.node], edge.kind});
				visit(edge.node, depth + 1);
				currentPath.pop_back();
				if (truncated && result.paths.size() >= limits.maximumPaths) break;
			}
			inPath[node] = false;
		};
		visit(*sourceIndex, 0);
		result.state = truncated ? OperationState::Truncated : OperationState::Complete;
		return result;
	}

	WhyCookedResult ExplainWhyCooked(
		const DependencyGraph& graph,
		std::span<const std::string_view> roots,
		std::string_view target,
		DependencyMask mask,
		OperationLimits limits)
	{
		WhyCookedResult result;
		std::vector<std::string> orderedRoots;
		orderedRoots.reserve(roots.size());
		for (const std::string_view root : roots) orderedRoots.emplace_back(root);
		std::sort(orderedRoots.begin(), orderedRoots.end());
		orderedRoots.erase(std::unique(orderedRoots.begin(), orderedRoots.end()), orderedRoots.end());
		if (orderedRoots.empty())
		{
			result.state = OperationState::Failed;
			result.error = {GraphErrorCode::UnknownNode, "why-cooked requires at least one root"};
			return result;
		}

		bool truncated = false;
		for (const std::string& root : orderedRoots)
		{
			PathResult candidate = FindShortestPath(graph, root, target, mask, limits);
			result.visitedNodes += candidate.visitedNodes;
			result.traversedEdges += candidate.traversedEdges;
			if (candidate.state == OperationState::Failed)
			{
				result.state = OperationState::Failed;
				result.error = std::move(candidate.error);
				return result;
			}
			if (candidate.state == OperationState::Truncated) truncated = true;
			if (!candidate.found) continue;

			const bool shorter = !result.found || candidate.steps.size() < result.steps.size();
			const bool sameLengthEarlierRoot = result.found && candidate.steps.size() == result.steps.size() && root < result.root;
			const bool sameRootEarlierPath = result.found && candidate.steps.size() == result.steps.size() && root == result.root &&
				std::lexicographical_compare(candidate.steps.begin(), candidate.steps.end(), result.steps.begin(), result.steps.end(), StepLess);
			if (shorter || sameLengthEarlierRoot || sameRootEarlierPath)
			{
				result.found = true;
				result.root = root;
				result.steps = std::move(candidate.steps);
			}
		}

		result.state = truncated ? OperationState::Truncated : OperationState::Complete;
		if (truncated)
		{
			result.found = false;
			result.root.clear();
			result.steps.clear();
		}
		return result;
	}

	CyclesResult FindCycles(const DependencyGraph& graph, DependencyMask mask, OperationLimits limits)
	{
		CyclesResult result;
		if (graph.nodes.size() > limits.maximumNodes)
		{
			result.state = OperationState::Truncated;
			return result;
		}
		const std::size_t unset = std::numeric_limits<std::size_t>::max();
		std::vector<std::size_t> index(graph.nodes.size(), unset);
		std::vector<std::size_t> low(graph.nodes.size(), unset);
		std::vector<std::size_t> stack;
		std::vector<bool> onStack(graph.nodes.size(), false);
		std::size_t nextIndex = 0;
		bool truncated = false;

		std::function<void(std::size_t, std::size_t)> strongConnect = [&](std::size_t node, std::size_t depth) {
			if (truncated) return;
			if (depth > limits.maximumDepth || result.visitedNodes >= limits.maximumNodes)
			{
				truncated = true;
				return;
			}
			index[node] = nextIndex;
			low[node] = nextIndex;
			++nextIndex;
			++result.visitedNodes;
			stack.push_back(node);
			onStack[node] = true;

			for (const GraphArc& edge : graph.outgoing[node])
			{
				if (!mask.Contains(edge.kind)) continue;
				if (result.traversedEdges >= limits.maximumEdges)
				{
					truncated = true;
					return;
				}
				++result.traversedEdges;
				if (index[edge.node] == unset)
				{
					strongConnect(edge.node, depth + 1);
					if (truncated) return;
					low[node] = std::min(low[node], low[edge.node]);
				}
				else if (onStack[edge.node])
				{
					low[node] = std::min(low[node], index[edge.node]);
				}
			}

			if (low[node] == index[node])
			{
				Cycle cycle;
				bool selfLoop = false;
				while (!stack.empty())
				{
					const std::size_t member = stack.back();
					stack.pop_back();
					onStack[member] = false;
					cycle.nodes.push_back(graph.nodes[member]);
					if (member == node) break;
				}
				if (cycle.nodes.size() == 1)
				{
					for (const GraphArc& edge : graph.outgoing[node])
					{
						if (edge.node == node && mask.Contains(edge.kind)) selfLoop = true;
					}
				}
				if (cycle.nodes.size() > 1 || selfLoop)
				{
					std::sort(cycle.nodes.begin(), cycle.nodes.end());
					result.cycles.push_back(std::move(cycle));
				}
			}
		};

		for (std::size_t node = 0; node < graph.nodes.size() && !truncated; ++node)
		{
			if (index[node] == unset) strongConnect(node, 0);
		}
		if (truncated)
		{
			result.state = OperationState::Truncated;
			result.cycles.clear();
			return result;
		}
		std::sort(result.cycles.begin(), result.cycles.end(), [](const Cycle& left, const Cycle& right) {
			return left.nodes < right.nodes;
		});
		return result;
	}
}
