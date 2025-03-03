import java.util.*;

public class FurthestFuturePaging {
    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        int instances = scanner.nextInt();

        for (int i = 0; i < instances; i++) {
            int cacheSize = scanner.nextInt();
            int numRequests = scanner.nextInt();
            int[] requests = new int[numRequests];
            for (int j = 0; j < numRequests; j++) {
                requests[j] = scanner.nextInt();
            }

            System.out.println(processInstance(cacheSize, requests));
        }
    }

    private static int processInstance(int cacheSize, int[] requests) {
        Set<Integer> cache = new HashSet<>();
        Map<Integer, Queue<Integer>> futureUses = new HashMap<>();
        int pageFaults = 0;

        for (int i = 0; i < requests.length; i++) {
            futureUses.putIfAbsent(requests[i], new LinkedList<>());
            futureUses.get(requests[i]).add(i);
        }

        for (int i = 0; i < requests.length; i++) {
            if (!cache.contains(requests[i])) {
                pageFaults++;
                if (cache.size() == cacheSize) {
                    int furthest = findFurthestPage(cache, futureUses, i);
                    cache.remove(furthest);
                }
                cache.add(requests[i]);
            }
            futureUses.get(requests[i]).poll();
        }

        return pageFaults;
    }

    private static int findFurthestPage(Set<Integer> cache, Map<Integer, Queue<Integer>> futureUses, int currentIndex) {
        int maxIndex = -1;
        int pageToReplace = -1;
        for (int page : cache) {
            Queue<Integer> uses = futureUses.get(page);
            int nextUse = uses.isEmpty() ? Integer.MAX_VALUE : uses.peek();
            if (nextUse > maxIndex) {
                maxIndex = nextUse;
                pageToReplace = page;
            }
        }
        return pageToReplace;
    }
}
