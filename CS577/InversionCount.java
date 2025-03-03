import java.util.Scanner;

public class InversionCount {
    private static long mergeAndCount(int[] arr, int l, int m, int r) {
        int[] left = new int[m - l + 1];
        int[] right = new int[r - m];
        System.arraycopy(arr, l, left, 0, left.length);
        System.arraycopy(arr, m + 1, right, 0, right.length);

        int i = 0, j = 0, k = l;
        long inversions = 0;

        while (i < left.length && j < right.length) {
            if (left[i] <= right[j]) {
                arr[k++] = left[i++];
            } else {
                arr[k++] = right[j++];
                inversions += (m + 1) - (l + i);
            }
        }

        while (i < left.length) {
            arr[k++] = left[i++];
        }

        while (j < right.length) {
            arr[k++] = right[j++];
        }

        return inversions;
    }

    private static long mergeSortAndCount(int[] arr, int l, int r) {
        long count = 0;
        if (l < r) {
            int m = (l + r) / 2;
            count += mergeSortAndCount(arr, l, m);
            count += mergeSortAndCount(arr, m + 1, r);
            count += mergeAndCount(arr, l, m, r);
        }
        return count;
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        int k = scanner.nextInt();
        while (k-- > 0) {
            int j = scanner.nextInt();
            int[] arr = new int[j];
            for (int i = 0; i < j; i++) {
                arr[i] = scanner.nextInt();
            }
            long inversions = mergeSortAndCount(arr, 0, arr.length - 1);
            System.out.println(inversions);
        }
        scanner.close();
    }
}
