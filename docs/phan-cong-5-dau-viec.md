# Week 3: ước lượng π bằng Monte Carlo đa luồng, phân công 5 đầu việc

Hạn nộp: **17:45 thứ Hai 02/11/2026**. Tệp này viết lúc 14:07 cùng ngày, nên nhóm còn khoảng 3 giờ 40 phút.

## 1. Nhóm phải làm gì

Đề yêu cầu viết **ba chương trình C dùng pthread** cùng tính π theo công thức `pi = 4 × (số điểm trong hình tròn) / (tổng số điểm)`, đo thời gian chạy rồi giải thích kết quả bằng kiến thức HĐH.

| Phần đề | Việc cụ thể |
|---|---|
| 3.1 Approach 1 | Một luồng, ≥ 10⁷ điểm, seed cố định, không in trong vòng lặp, chạy nhiều lần lấy trung bình. Đây là mốc `T_single` |
| 3.2 Approach 2 | N luồng, mỗi luồng làm `nPoints/N` điểm, **đếm vào biến cục bộ**, mỗi luồng một bộ sinh số ngẫu nhiên riêng, join xong mới cộng lại (map–reduce). Đo với N = 2, 4, 8, … đến ≥ 100 |
| 3.3 Approach 3 | Như Approach 2 nhưng mọi luồng cộng vào **một biến toàn cục có khoá mutex**. Kết quả vẫn đúng nhưng phải chậm hơn |
| 4 Đo đạc | Mỗi cấu hình ≥ 5 lần, lấy trung bình; giữ nguyên số điểm khi đổi số luồng; không tính khởi tạo và I/O vào thời gian; máy không chạy việc nặng khác; đổi tham số nào thì ghi lại |
| 5.1 Đồ thị | Với mỗi approach: thời gian theo số luồng và speedup theo số luồng, `Speedup = T_single / T_parallel` |
| 5.2 Giải thích | A. speedup không tăng tuyến tính · B. điểm bão hoà và quan hệ với số core · C. oversubscription (luồng nhiều hơn core, context switch, định thời) · D. vì sao mutex làm Approach 3 chậm · E. vì sao hiệu suất `E = Speedup / N` giảm |
| 6 Thí nghiệm đánh đổi | Bắt buộc **ít nhất một** trong ba: 6.1 cộng vào biến chung mỗi vòng lặp so với mỗi k vòng · 6.2 số luồng dưới, bằng, trên số core · 6.3 cộng cục bộ so với cộng vào biến chung |
| 7 Nộp | Mã nguồn cả ba approach · số đo · đồ thị (thời gian, speedup, hiệu suất nếu muốn) · phần phân tích · danh sách best practice rút ra từ kết quả |

Đồ thị mẫu của thầy (Throughput theo số luồng, máy 4 core) có dạng **tăng tới 4 luồng, tụt ở 8, rồi nhích lên và đi ngang**. Đó là hình dạng tham khảo; nhóm vẽ đúng số mình đo được, không chỉnh cho giống mẫu.

Đề cho chọn một thí nghiệm ở mục 6. Đề nghị làm cả ba: 6.2 và 6.3 có sẵn từ số đo chính, chỉ 6.1 tốn thêm một vòng chạy.

### Bốn bẫy làm hỏng số đo, cả nhóm phải biết

1. **Không dùng `rand()`.** Trong glibc, `rand()` dùng chung một trạng thái có khoá bên trong, nên Approach 2 sẽ chậm đi khi tăng luồng và cả bài sai bản chất. Dùng `rand_r(&seed)` với `seed` riêng từng luồng.
2. **Không đo bằng `clock()`.** `clock()` cộng thời gian CPU của **mọi** luồng, nên chạy 4 luồng sẽ ra số lớn gần gấp 4. Dùng `clock_gettime(CLOCK_MONOTONIC, …)` để đo thời gian thực.
3. **Không đặt bộ đếm cục bộ trong mảng `count[tid]` rồi `++` ở mỗi vòng.** Các phần tử nằm chung cache line nên các core giành nhau dòng cache (false sharing). Đếm vào biến cục bộ trên stack của hàm luồng, cuối hàm mới ghi ra một lần.
4. **Tất cả số đo chạy trên cùng một máy, cùng cờ biên dịch `-O2 -pthread`.** Ghi lại `nproc` và `lscpu`. Nếu dùng VM thì số core là số vCPU của VM.

## 2. Giao ước chung: chốt trước khi chia việc

Năm người làm song song, nên ai cũng phải theo đúng giao ước này. Có nó thì không ai phải chờ người khác viết xong mới bắt đầu.

**Cấu trúc thư mục nộp**

```
week3-pi/
├── src/common.h        # Việc 1
├── src/pi_single.c     # Việc 1
├── src/pi_local.c      # Việc 2
├── src/pi_mutex.c      # Việc 3
├── Makefile            # Việc 1
├── scripts/run_bench.sh  # Việc 4
├── scripts/plot.py       # Việc 4
├── results/            # Việc 4: raw.csv, summary.csv, machine.txt
├── figures/            # Việc 4: *.png
├── report.pdf          # Việc 5
└── README.md           # Việc 1
```

**Dòng lệnh** (sai số lượng hoặc kiểu tham số thì in usage ra stderr, trả mã 1):

```
./pi_single <nPoints> <seed>
./pi_local  <nPoints> <nThreads> <seed>
./pi_mutex  <nPoints> <nThreads> <seed> [k]     # k mặc định 1
```

**Đầu ra**: đúng **một dòng CSV** ra stdout, không in gì khác:

```
approach,threads,k,points,inside,pi,time_sec
local,8,0,10000000,7853912,3.14156480,0.041234
```

`approach` là `single`, `local` hoặc `mutex`. Cột `k` ghi 0 cho hai approach không có k. Số trong dòng ví dụ chỉ minh hoạ định dạng.

**Hàm dùng chung trong `common.h`** (Việc 1 viết; Việc 2 và 3 gọi đúng tên này). Không có `common.c`, nên dòng đầu tệp là `#define _POSIX_C_SOURCE 200809L` (để `clock_gettime` và `rand_r` được khai báo) và cả ba hàm khai báo `static inline` (để `-Wall` không báo hàm không dùng):

```c
double now_sec(void);                         // clock_gettime(CLOCK_MONOTONIC) đổi ra giây
double rand01(unsigned int *seed);            // rand_r(seed) / (double)RAND_MAX, trả [0,1]
void   print_result(const char *approach, int threads, long k,
                    long long points, long long inside, double sec);
```

**Quy ước tính**

- Điểm lấy trong góc phần tư `x, y ∈ [0,1]`, nằm trong đường tròn khi `x*x + y*y <= 1`. Cách này cho cùng tỉ lệ π/4 như hình vuông [-1,1] trong đề; ghi một câu giải thích trong báo cáo.
- Chia việc: `base = nPoints / T`; `T` không chia hết thì `nPoints % T` luồng đầu mỗi luồng làm thêm 1 điểm, để tổng luôn đúng `nPoints`.
- Seed: luồng thứ `tid` (0 … T−1) dùng `seed + tid`. Approach 1 dùng `seed`.
- Đo thời gian: bắt đầu **ngay trước vòng `pthread_create`**, kết thúc **sau `pthread_join` cuối cùng và bước cộng kết quả**. Phân tích tham số và `printf` nằm ngoài khoảng đo. Tạo và join luồng là chi phí thật của song song nên được tính; ghi rõ điều này trong báo cáo.
- Kích thước: **`nPoints = 10000000` (10⁷, đúng mức tối thiểu của đề), seed = 42**. Approach 3 khoá mỗi vòng lặp nên 10⁸ điểm có thể mất hàng chục giây mỗi lần, nhân với khoảng 90 lần chạy là không kịp hạn. Nếu chạy hết bộ đo còn dư thời gian thì có thể đo lại toàn bộ với 10⁸, nhưng ba approach phải cùng một kích thước.
- Số luồng: **1, 2, 4, 8, 16, 32, 64, 128**, cộng thêm đúng `nproc` nếu nó không phải luỹ thừa của 2.

**Phép tự kiểm tra đúng sai mạnh nhất:** cùng `T` và cùng seed thì `pi_local` và `pi_mutex` (mọi `k`) phải ra **cột `inside` giống hệt nhau**, vì hai chương trình sinh cùng một dãy điểm. Lệch dù một đơn vị là có race condition hoặc chia việc sai.

## 3. Năm đầu việc

**Việc 5 đã giao cho Tuấn Anh (@PTAxHVA)**, kèm việc review và merge mọi PR. Bốn thành viên còn lại tự chọn Việc 1–4, mỗi người một việc. Mọi thay đổi đi qua branch và PR theo `AGENTS.md`.

### Việc 1: Approach 1, thư viện chung, Makefile, README và đóng gói

Đây là việc nhẹ nhất về code nhưng nằm trên đường găng, nên người nhận việc này cũng lo khâu tích hợp và nộp bài.

**Input:** mục 3.1 và mục 4 của đề, giao ước ở mục 2 của tệp này, một máy Linux có `gcc` (VM Ubuntu 24.04 của lab là đủ).

**Cần làm**

1. Viết `src/common.h` với ba hàm `now_sec`, `rand01`, `print_result` theo đúng chữ ký ở mục 2. **Đẩy lên nhóm trước 14:45** để Việc 2 và 3 ghép vào.
2. Viết `src/pi_single.c`: kiểm tham số → `t0 = now_sec()` → vòng lặp `nPoints` lần sinh `x, y` bằng `rand01(&seed)`, đếm `inside` → `t1 = now_sec()` → `print_result("single", 1, 0, …)`. Không in gì trong vòng lặp.
3. Viết `Makefile`: đích `all` (đích đầu tiên) dựng ba chương trình với `gcc -O2 -Wall -pthread`, cộng đích `clean`. Mọi approach phải dùng chung một bộ cờ.
4. Chạy thử `./pi_single 10000000 42` ba lần: π phải lệch 3.14159 dưới 0.002, thời gian ba lần gần nhau.
5. Sau khi Việc 2 và 3 nộp code: `make clean && make` không có cảnh báo; chạy kiểm tra cột `inside` ở mục 2 cho T = 1, 4, 7, 128.
6. Viết `README.md`: cách build, cách chạy từng chương trình, cách chạy `run_bench.sh` và `plot.py`, thông tin máy đo.
7. Gom toàn bộ thư mục `week3-pi/` cùng `report.pdf` thành tệp zip, đặt tên theo quy định của thầy, rồi nộp **trước 17:30**.

**Output:** `src/common.h`, `src/pi_single.c`, `Makefile`, `README.md`, tệp zip nộp bài.

**Xong khi:** `make` sạch; `pi_single` cho π đúng; phép kiểm cột `inside` khớp ở cả bốn giá trị T; zip mở ra dựng và chạy được trên máy khác.

### Việc 2: Approach 2, cộng cục bộ (map–reduce)

**Input:** mục 3.2 của đề, giao ước ở mục 2, `common.h` của Việc 1. Trong lúc chờ thì tự viết tạm ba hàm theo đúng chữ ký, sau đó thay bằng bản thật.

**Cần làm**

1. Định nghĩa struct tham số cho mỗi luồng gồm `tid`, `nPoints` riêng, `seed` riêng và chỗ trả kết quả `inside`.
2. Hàm luồng: `unsigned int s = seed;` và `long long local = 0;` là **biến cục bộ**. Vòng lặp chỉ đụng tới `s` và `local`; hết vòng mới ghi `local` vào struct **một lần**. Trong vòng lặp không có biến chung, khoá hay `printf`.
3. `main`: kiểm tham số (`nThreads ≥ 1`, `nPoints ≥ nThreads`) → chia việc theo quy ước phần dư → `t0` → tạo T luồng → join đủ T → cộng các `inside` (pha reduce) → `t1` → `print_result("local", T, 0, …)`.
4. Kiểm lỗi: `pthread_create` trả khác 0 thì in lỗi kèm số luồng đang tạo, rồi thoát mã 1. Ở 128 luồng trên VM vẫn phải chạy được.
5. Tự thử với T = 1, 2, 4, 8, 128: π đúng, tổng số điểm đúng `nPoints`, thời gian giảm dần cho tới khoảng `nproc`.
6. Viết nửa trang mô tả thiết kế cho báo cáo: vì sao không cần khoá, vì sao mỗi luồng một seed, vì sao không đếm vào `count[tid]` (false sharing), pha map và pha reduce nằm ở đâu trong code.
7. **Sau 15:15**, khi có số của Việc 4: viết mục **A, B, C** và thí nghiệm **6.2** của báo cáo, có số đo thật, nộp bằng PR thêm file `docs/report/task2-analysis-abc-6.2.md` trước 16:50.

**Output:** `src/pi_local.c`, đoạn mô tả thiết kế và `docs/report/task2-analysis-abc-6.2.md`.

**Xong khi:** biên dịch không cảnh báo; với cùng T và seed, cột `inside` khớp `pi_mutex`; chạy được 128 luồng.

### Việc 3: Approach 3, biến chung có mutex, và thí nghiệm 6.1 (tham số k)

**Input:** mục 3.3 và mục 6.1 của đề, giao ước ở mục 2, `common.h`.

**Cần làm**

1. Biến toàn cục `long long g_inside = 0;` và `pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;`.
2. Hàm luồng nhận thêm tham số `k`:
   - Một vòng lặp duy nhất cho mọi `k ≥ 1`: đếm điểm trúng vào biến cục bộ `batch`; cứ hết k vòng lặp thì `lock → g_inside += batch → unlock` rồi đặt `batch = 0`; hết vòng thì cộng nốt phần còn lại.
   - `k = 1` là bản chuẩn của Approach 3: **mỗi vòng lặp khoá một lần**, đúng chữ "update the shared variable every iteration" của mục 6.1. `k > 1` là thí nghiệm 6.1. Một đường code cho mọi k thì so sánh giữa các k mới công bằng.
   - Seed riêng `seed + tid` và cách chia việc giữ đúng như Approach 2, để phép kiểm cột `inside` có nghĩa.
3. `main`: kiểm tham số (`k ≥ 1`), tạo và join luồng, đo thời gian theo quy ước, `print_result("mutex", T, k, …)`. Ở cuối gọi `pthread_mutex_destroy`.
4. Kiểm cột `inside`: cùng T và seed thì `pi_mutex` với k = 1, 10, 1000 phải ra đúng số của `pi_local`.
5. Làm một bản thử **cố ý bỏ khoá** (không đưa vào bài nộp) chạy với 8 luồng: `inside` sẽ hụt và π sai. Ghi lại một dòng số liệu thật làm bằng chứng cho phần "race condition" của báo cáo.
6. Viết nửa trang cho báo cáo: critical section nằm ở đâu, vì sao chạy nhiều luồng mà vẫn bị tuần tự hoá (serialization), k lớn dần thì Approach 3 tiến về Approach 2 ra sao.
7. **Sau 15:15**, khi có số của Việc 4: viết mục **D, E** và các thí nghiệm **6.1, 6.3** của báo cáo, có số đo thật, nộp bằng PR thêm file `docs/report/task3-analysis-de-6.1-6.3.md` trước 16:50.

**Output:** `src/pi_mutex.c`, dòng số liệu của bản không khoá, đoạn mô tả và `docs/report/task3-analysis-de-6.1-6.3.md`.

**Xong khi:** cột `inside` khớp `pi_local` với mọi k đã thử; chạy được 128 luồng; không còn `rand()` và `clock()` trong code.

### Việc 4: chạy đo và vẽ đồ thị

Người nhận việc này giữ **cái máy đo**. Toàn bộ số liệu đi ra từ một máy duy nhất.

**Input:** mục 4, 5.1 và 6 của đề; ba binary của Việc 1–3. Trước khi có binary thì viết script và chạy thử bằng bản nháp của chính mình, **không bịa số**.

**Cần làm**

1. Ghi thông tin máy vào `results/machine.txt`: kết quả của `nproc`, `lscpu`, `uname -a`, `gcc --version`, máy thật hay VM và bao nhiêu vCPU.
2. Viết `scripts/run_bench.sh`:
   - đặt `N=10000000`, `SEED=42`, `RUNS=5`, danh sách luồng như ở mục 2;
   - **chạy theo thứ tự này** để có số sớm: `pi_single` × 5 → `pi_local` × mọi số luồng × 5 → gửi ngay hai phần này cho Việc 2 và 5 → `pi_mutex` (k = 1) × mọi số luồng × 5 → 6.1;
   - mỗi cấu hình của `pi_single` và `pi_local` chạy **1 lần khởi động bỏ đi** trước 5 lần ghi lại; `pi_mutex` bỏ lần khởi động để tiết kiệm thời gian;
   - 6.1: `pi_mutex` với **T = nproc** và k = 10, 100, 1 000, 10 000, 100 000 × 5. Không chạy lại k = 1, vì cấu hình (T = nproc, k = 1) đã có ở bước trước;
   - nối mọi dòng CSV vào `results/raw.csv`, kèm dòng tiêu đề;
   - tham số sai thì in usage, không chạy gì.
3. Chạy thử `pi_mutex 10000000 128 42` một lần để ước thời gian cả bộ đo. Nếu một lần quá 10 giây thì báo nhóm: giữ 10⁷ nhưng bỏ bớt cấu hình 6.1 (giữ k = 1, 100, 10 000).
4. Chạy đo thật. Tắt trình duyệt và các ứng dụng nặng; nếu dùng VM thì trên máy Mac cũng đóng ứng dụng nặng. Ghi giờ bắt đầu và kết thúc.
5. Viết `scripts/plot.py` (pandas + matplotlib):
   - đọc `raw.csv`; với mỗi (approach, threads, k) tính trung bình và độ lệch chuẩn → `results/summary.csv`;
   - `speedup = T_single_tb / T_tb`, `efficiency = speedup / threads`;
   - ba hình dưới đây chỉ lấy dòng `mutex` có **`k == 1`**; hình 6.1 chỉ lấy `mutex` có `threads == nproc`;
   - `figures/time.png`: thời gian theo số luồng, hai đường local và mutex, trục x log₂, có error bar, thêm một đường ngang cho `T_single`;
   - `figures/speedup.png`: speedup của local và mutex, thêm đường lý tưởng `y = x` và đường dọc tại `nproc`;
   - `figures/efficiency.png`: hiệu suất theo số luồng;
   - `figures/sync_k.png` (6.1): thời gian theo k, trục x log₁₀, thêm đường ngang cho thời gian của local ở cùng T;
   - mọi hình đều ghi trên tiêu đề hoặc chú thích: số điểm, số lần chạy, số core.
6. Gửi số `single` + `local` ngay khi đo xong (khoảng 15:40); gửi `summary.csv` đầy đủ cùng bốn hình **trước 16:15**.

**Output:** `scripts/run_bench.sh`, `scripts/plot.py`, `results/{machine.txt, raw.csv, summary.csv}`, `figures/*.png`.

**Xong khi:** `raw.csv` có đủ 5 dòng cho mỗi cấu hình, không cấu hình nào bị đo hai lần; chạy lại `plot.py` ra đúng các hình; mọi giá trị π trong `raw.csv` lệch 3.14159 dưới 0.002.

### Việc 5: báo cáo phân tích và best practice, kiêm review code (Tuấn Anh)

Người này là **người biên tập** báo cáo: viết khung, phần lý thuyết và best practice, rồi ghép các mục A–E, 6.1–6.3 do Việc 2 và Việc 3 viết. Phần lý thuyết viết được ngay, không cần chờ số.

**Input:** mục 1, 5.2, 6 và 7 của đề; `summary.csv` và hình của Việc 4; các đoạn mô tả của Việc 2 và 3. Lý thuyết dẫn từ slide môn: `Lectures/ch4_EN_BK_Threads_2025.pdf` (hai slide `Amdahl's Law`, multicore), `Lectures/ch5_EN_CPUSched_2024.pdf` (context switch, định thời), `Lectures/ch6_EN_BK_syn1.pdf` (race condition, critical section, mutex lock).

**Cần làm**

1. Dựng khung báo cáo: Mục tiêu · Môi trường đo (chép `machine.txt`) · Phương pháp (giao ước mục 2: kích thước, seed, số lần chạy, đo cái gì và bỏ cái gì) · Thiết kế ba approach · Kết quả (bảng + bốn hình) · Phân tích A–E · Thí nghiệm đánh đổi · Best practice · Phân công thành viên · Ghi chú dùng AI.
2. Viết trước, không cần số, phần lý thuyết của A–E:
   - A. Định luật Amdahl, phần tuần tự (tạo/join luồng, reduce), giới hạn băng thông bộ nhớ.
   - B. Số luồng chạy thật sự song song không vượt quá số core.
   - C. Khi luồng nhiều hơn core: bộ định thời chia time slice, chi phí context switch, cache bị xoá dần, chi phí tạo luồng.
   - D. Critical section, contention, luồng ngủ chờ khoá, tuần tự hoá.
   - E. `E = S / N`, vì sao luôn ≤ 1 và giảm dần.
3. Khi nhận mục A–E và 6.1–6.3 từ Việc 2, 3 (trước 16:50): ghép vào khung, kiểm mỗi ý dẫn **số đo cụ thể** từ `summary.csv`, ví dụ "speedup ở 4 luồng là … so với lý tưởng 4". Chỉ ra điểm bão hoà thật trên đồ thị và so với `nproc`. Đối chiếu với đồ thị mẫu của thầy, nói chỗ nào giống, chỗ nào khác, và vì sao.
4. Soát phần 6.3 có dùng đúng ba chữ đề nêu: critical section · contention · serialization.
5. Viết 5–8 best practice, mỗi cái **neo vào một con số của nhóm**. Ví dụ: cộng cục bộ rồi reduce · số luồng ≈ số core · mỗi luồng một bộ sinh ngẫu nhiên riêng, không dùng `rand()` · giảm tần suất đồng bộ · tránh false sharing · đo bằng đồng hồ thật, chạy nhiều lần.
6. Ghi bảng phân công 5 người và câu khai báo dùng AI theo đề cương môn: được dùng để tham khảo, phải ghi rõ công cụ.
7. Xuất `report.pdf`, gửi Việc 1 **trước 17:15**.

**Output:** `report.pdf` (đề nghị 6–10 trang).

**Xong khi:** đủ năm ý A–E, mỗi ý có ít nhất một con số đo thật; có đủ bốn hình; có phần trade-off và best practice; mọi số trong báo cáo khớp `summary.csv`.

#### Review PR: soát gì ở từng PR

Mọi PR tự gán reviewer là @PTAxHVA. Mỗi PR soát trong 5–10 phút, rồi Approve và merge, hoặc Request changes kèm dòng cần sửa.

Mọi PR:
- Chỉ đổi file của đúng việc đó (bảng trong `README.md`); thân PR có đủ Task, Files changed, How I tested.
- Không có file chạy hay `*.o` bị commit; `make clean && make` không cảnh báo.

PR code (Việc 1, 2, 3):
- Không còn `rand()` và `clock()`; có `rand_r` với seed riêng `seed + tid`; đo bằng `CLOCK_MONOTONIC`.
- Khoảng đo bắt đầu trước `pthread_create`, kết thúc sau join và reduce; `printf` nằm ngoài khoảng đo.
- Đầu ra đúng một dòng CSV theo mục 2; tham số sai thì in usage và trả mã 1.
- Phần dư `nPoints % T` được chia, tổng điểm đúng `nPoints`.
- `pi_local`: vòng lặp không đụng biến chung, không đếm vào `count[tid]`.
- `pi_mutex`: một vòng lặp cho mọi `k`, lock/unlock bao đúng phép cộng, cộng nốt `batch` sau vòng.
- Tự chạy trên máy mình: `./pi_local 10000000 4 42` và `./pi_mutex 10000000 4 42` (rồi thêm `10`) phải ra **cùng cột `inside`**.

PR đo (Việc 4):
- `machine.txt` có `nproc`, `lscpu` và ghi rõ máy thật hay VM.
- `raw.csv` đủ 5 dòng mỗi cấu hình, không cấu hình nào đo hai lần, mọi π lệch 3.14159 dưới 0.002.
- Ba hình đầu chỉ lấy `mutex` có `k == 1`; hình 6.1 lấy `threads == nproc`; mọi hình ghi số điểm, số lần chạy, số core.

PR phân tích (Việc 2, 3):
- Mỗi ý có ít nhất một con số, và con số đó có trong `summary.csv`.

## 4. Mốc thời gian (hôm nay, 02/11)

| Giờ | Mốc | Ai |
|---|---|---|
| 14:15 | Cả nhóm đọc mục 2, chốt giao ước, chọn máy đo, mỗi người nhận một việc | Cả nhóm |
| 14:45 | PR `common.h` + Makefile | Việc 1 |
| 14:55 | Review và merge PR `common.h` để Việc 2, 3 kéo về | Tuấn Anh |
| 15:15 | PR `pi_local.c`, `pi_mutex.c`, `pi_single.c` | Việc 1, 2, 3 |
| 15:25 | Review, chạy phép kiểm cột `inside`, merge; Việc 4 kéo `main` về máy đo | Tuấn Anh |
| 15:15–16:10 | Chạy đo; số `single` + `local` gửi ra khoảng 15:40 | Việc 4 |
| 16:15 | PR `results/` + `figures/` + hai script | Việc 4 |
| 16:25 | Review và merge PR số đo | Tuấn Anh |
| 16:50 | PR `docs/report/task2-…md` và `task3-…md` | Việc 2, 3 |
| 16:55 | Review, merge, ghép vào báo cáo | Tuấn Anh |
| 17:15 | `report.pdf` xong | Việc 5 |
| 17:30 | Nộp zip | Việc 1 |

Trong lúc máy đang chạy, Việc 4 viết `plot.py`. Việc 1 đọc soát báo cáo trước khi xuất PDF, vì Tuấn Anh vừa viết vừa review nên cần một người đọc thứ hai. Giữa các lượt review, Tuấn Anh viết phần lý thuyết của báo cáo.

## 5. Còn để ngỏ

- Thầy có quy định tên tệp nộp hay ngôn ngữ báo cáo (Anh hay Việt) không? Đề không ghi.
- Thầy dặn cả nhóm đo trên **một máy**, và bản phân công đã theo đúng như vậy: mọi người tự code và thử trên máy mình, còn số đo chính thức chỉ do Việc 4 chạy trên máy đó. Nhóm chỉ còn phải chọn máy nào (VM 4 vCPU của lab hay một máy Linux thật nhiều core hơn).
