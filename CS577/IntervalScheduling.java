import java.util.*;

public class IntervalScheduling {
    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        int k = scanner.nextInt(); //read number of instances

        for (int instance = 0; instance < k; instance++) {
            int n = scanner.nextInt(); //read number of jobs for this instance
            Job[] jobs = new Job[n];

            for (int i = 0; i < n; i++) {
                int start = scanner.nextInt();
                int end = scanner.nextInt();
                jobs[i] = new Job(start, end);
            }

            //sort jobs by end time
            Arrays.sort(jobs, Comparator.comparingInt(job -> job.end));

            //select jobs using the greedy algorithm
            int count = 0;
            int lastEndTime = 0;

            for (Job job : jobs) {
                if (job.start >= lastEndTime) {
                    lastEndTime = job.end;
                    count++;
                }
            }

            //output the result for this instance
            System.out.println(count);
        }

        scanner.close();
    }

    static class Job {
        int start;
        int end;

        Job(int start, int end) {
            this.start = start;
            this.end = end;
        }
    }
}
