// Phoenix: Native x86 Decompilation Using Semantics-Preserving Structural Analysis and Iterative Control-Flow Structuring




#include "backend/phoenix.h"
#include <iostream>
namespace notdec::backend {

Region Phoenix::execute() {
    // BuildRegionGraph
    graph.build();

    int iterations = 0;
    int oldCount;
    int newCount;
    do {
        if (isCanceled) {
            break;
        }
        ++iterations;
        if (iterations > 1000)
        {
            std::cerr << "Structure analysis stopped making progress, quitting."
                << " Func: " << func.getName().str();
            break;
        }

        oldCount = regionGraph.Nodes.Count;
        this.doms = new DominatorGraph<Region>(this.regionGraph, this.entry);
        this.unresolvedCycles = new Queue<(Region, ISet<Region>)>();
        this.unresolvedSwitches = new Queue<Region>();
        var postOrder = new DfsIterator<Region>(regionGraph).PostOrder(entry).ToList();

        foreach (var n in postOrder)
        {
            Probe();
            bool didReduce;
            do
            {
                if (eventListener.IsCanceled())
                    break;
                didReduce = ReduceAcyclic(n);
                if (!didReduce && IsCyclic(n))
                {
                    didReduce = ReduceCyclic(n);
                }
            } while (didReduce);
        }
        newCount = regionGraph.Nodes.Count;
        if (newCount == oldCount && newCount > 1)
        {
            // Didn't make any progress this round,
            // try refining unstructured regions
            ProcessUnresolvedRegions();
        }
    } while (regionGraph.Nodes.Count > 1);
    return entry;
}

}