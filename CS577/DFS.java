import java.util.*;

public class DFS {

    static class Graph {
        private List<List<Integer>> adjacencyList;
        private List<String> nodeLabels;
        private Map<String, Integer> nodeIndices;

        public Graph() {
            adjacencyList = new ArrayList<>();
            nodeIndices = new HashMap<>();
            nodeLabels = new ArrayList<>();
        }

        public void addNode(String label) {
            if (!nodeIndices.containsKey(label)) {
                nodeIndices.put(label, nodeLabels.size());
                nodeLabels.add(label);
                adjacencyList.add(new ArrayList<>());
            }
        }

        public void addEdge(String from, String to) {
            int fromIndex = nodeIndices.get(from);
            int toIndex = nodeIndices.get(to);
            adjacencyList.get(fromIndex).add(toIndex);
        }

        public List<Integer> getNeighbors(int nodeIndex) {
            return adjacencyList.get(nodeIndex);
        }

        public String executeDFS() {
            Set<Integer> visitedNodes = new HashSet<>();
            StringBuilder traversalResult = new StringBuilder();
            int startIndex = 0;

            while (startIndex < nodeLabels.size()) {
                while (startIndex < nodeLabels.size() && visitedNodes.contains(startIndex)) {
                    startIndex++;
                }
                if (startIndex >= nodeLabels.size()) {
                    break;
                }
                Deque<Integer> stack = new ArrayDeque<>();
                stack.push(startIndex);

                while (!stack.isEmpty()) {
                    int currentNode = stack.pop();
                    while (visitedNodes.contains(currentNode) && !stack.isEmpty()) {
                        currentNode = stack.pop();
                    }
                    if (visitedNodes.contains(currentNode)) {
                        break;
                    }
                    visitedNodes.add(currentNode);
                    traversalResult.append(nodeLabels.get(currentNode)).append(" ");
                    List<Integer> neighbors = getNeighbors(currentNode);
                    for (int i = neighbors.size() - 1; i >= 0; i--) {
                        int neighbor = neighbors.get(i);
                        if (!visitedNodes.contains(neighbor)) {
                            stack.push(neighbor);
                        }
                    }
                }
                startIndex++;
            }
            return traversalResult.toString().trim();
        }
    }

    static Graph[] graphCollection;

    public static void main(String[] args) {
        initializeGraphs();
        for (Graph graph : graphCollection) {
            System.out.println(graph.executeDFS());
        }
    }

    public static void initializeGraphs() {
        Scanner scanner = new Scanner(System.in);
        int numberOfGraphs = scanner.nextInt();
        graphCollection = new Graph[numberOfGraphs];

        for (int i = 0; i < numberOfGraphs; i++) {
            int numberOfNodes = scanner.nextInt();
            scanner.nextLine();

            Graph graph = new Graph();
            List<String> inputLines = new ArrayList<>();

            for (int j = 0; j < numberOfNodes; j++) {
                String line = scanner.nextLine();
                inputLines.add(line);
                String[] parts = line.split(" ");
                graph.addNode(parts[0]);
            }

            for (String line : inputLines) {
                String[] parts = line.split(" ");
                String sourceNode = parts[0];
                for (int k = 1; k < parts.length; k++) {
                    graph.addNode(parts[k]);
                    graph.addEdge(sourceNode, parts[k]);
                }
            }
            graphCollection[i] = graph;
        }
        scanner.close();
    }
}
