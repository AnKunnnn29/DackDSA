# Báo cáo tích hợp hệ thống quản lý kho và đơn hàng

Ngày thực hiện: **03/10/2026**. Nhánh kết quả: **`merge`**. Ngôn ngữ: **C++17**.

Bản này ghép chức năng các nhánh thành một chương trình console có dữ liệu chung, sửa các khác biệt nghiệp vụ và bổ sung lưu/đọc CSV. Nhánh `main` và các nhánh chức năng gốc giữ nguyên. Các merge commit giữ lịch sử commit nguồn; sau đó có commit chỉnh sửa tích hợp.

## 1. Phân công nhóm và tình trạng nguồn

| Thành viên | Phạm vi theo phân công người dùng cung cấp | File chức năng trong bản tích hợp |
|---|---|---|
| An | Tra cứu sản phẩm theo mã sản phẩm; tạo và quản lý đơn hàng | `ProductLookup.cpp`, `OrderManager.cpp` |
| Nam | Quản lý thông tin sản phẩm; quản lý số lượng hàng | `ProductManagement.cpp`; tái sử dụng kiểm tra sản phẩm ở `ProductLookup.cpp` |
| Đình Hải | Tra cứu theo mã đơn hàng; tìm sản phẩm theo khoảng giá | `OrderLookup.cpp`, `PriceSearch.cpp` |
| Đăng Khoa | Xử lý đơn theo ưu tiên; cảnh báo sắp hết hàng | `PriorityOrders.cpp`, `LowStock.cpp` |
| Huỳnh Khoa | Theo dõi trạng thái đơn hàng | `OrderStatus.cpp`; node lịch sử chung trong `Models.h` |

Mỗi thành viên phụ trách review và kiểm chứng các chức năng được phân công, đồng thời hoàn thiện phần biện minh thiết kế cá nhân.

Nguồn đối chiếu trước khi tích hợp:

| Nhánh nguồn | Commit | Nội dung nguồn và xử lý |
|---|---|---|
| `feature/product-lookup-order-management` | `516ccb6` | Nền tảng hash, tạo đơn, model và console thuộc phần An; giữ và mở rộng |
| `Product` | `3e3ee8e` | Chỉ có khung chung, chưa có implementation quản lý sản phẩm/tồn kho; **bản tích hợp bổ sung mới phạm vi này** |
| `Tra-cứu-sản-phẩm-theo-mã` | `3df1de3` | Binary search Product ID, trùng phạm vi của An; dùng hash sản phẩm làm bản chính và tạo `OrderLookup.cpp` để khớp phân công Đình Hải |
| `Tìm-kiếm-sản-phẩm-theo-khoảng-giá` | `4704a12` | Giữ ý tưởng tìm hai biên bằng binary search; bổ sung chỉ mục giá và trả dữ liệu |
| `Xử-lý-đơn-hàng-theo-mức-độ-ưu-tiên` | `30b891e` | Chỉnh quy tắc ưu tiên và thay heap thư viện bằng heap tự cài đặt |
| `Cảnh-báo-sản-phẩm-sắp-hết` | `30169e6` | Đổi ngưỡng 15% sang `minStock` của từng sản phẩm |
| `Theo-dõi-trạng-thái-đơn-hàng` | `6d94d75` | Giữ luồng transition và ý tưởng lịch sử linked list; đổi model/store, sửa sở hữu bộ nhớ, bổ sung hoàn kho |

Các file nguồn cũ `Tracuutheoma.cpp`, `Timkiemtheokhoanggia.cpp`, `theodoitrangthaidonhang.cpp`, `mucdouutiendebuild/douutien.cpp`, `saphethangdebuild/saphethang.cpp` được thay bằng các module nêu trên. Bản gốc vẫn đọc được qua commit/nhánh nguồn, không còn hai implementation cùng được build.

## 2. Kiến trúc và dữ liệu chung

Luồng hoạt động:

```text
main.cpp                     Giao diện console
  -> Các service             DSA Core và nghiệp vụ
     -> ProductStore         Một kho sản phẩm dùng chung
     -> OrderStore           Một kho đơn hàng dùng chung
  -> FileStorage             Load/save snapshot CSV
```

- `main.cpp` là nơi duy nhất hiển thị menu và kết quả. Các service trả dữ liệu hoặc `Result`, không in thông báo nghiệp vụ trực tiếp.
- `FileStorage.cpp` chỉ lưu/đọc dữ liệu. Tra cứu, khoảng giá, ưu tiên và trạng thái vẫn thực hiện trên cấu trúc trong bộ nhớ.
- Các `.cpp` chức năng có include guard. Do nhóm chọn gộp header vào `.cpp`, **chỉ compile `main.cpp`** cho chương trình chính. Cấu hình hiện tại chỉ tạo executable dsa_demo.
- Khi tích hợp web nhiều file biên dịch riêng, cần tách lại giao diện `.h`. Hiện chưa có web/API.

### Các thay đổi trong Models.h

| Nội dung | Trước tích hợp | Sau tích hợp |
|---|---|---|
| Product/Order | Một số module tự định nghĩa model toàn cục | Dùng model duy nhất trong namespace `shop` |
| Tiền và số lượng | Có module dùng `double`/`int` | `Money` và `Quantity` là số nguyên 64-bit; tiền tính bằng VNĐ |
| Thời gian | Module ưu tiên dùng chuỗi | `createdAt` và `changedAt` dùng Unix milliseconds |
| Trạng thái | Có `Status` hoặc enum toàn cục riêng | `enum class OrderStatus` chung |
| Lịch sử | Raw pointer `HistoryNode*`, không giải phóng an toàn | Chuỗi node bất biến qua `shared_ptr<const StatusHistoryNode>` |
| Lỗi | Một số hàm chỉ in console | `ErrorCode`/`Result` chung cho lỗi nghiệp vụ, dữ liệu và I/O |

Hai store có `revision`. Mọi service thay đổi dữ liệu tăng revision để chỉ mục giá hoặc heap nhận biết dữ liệu đã đổi. Không sửa trực tiếp trường/bản ghi từ module khác mà bỏ qua cơ chế này. Load CSV tăng revision của store hiện tại để cache cũ được làm mới.

## 3. Phần An

### Tra cứu sản phẩm theo mã

Giữ `HashTable<T>` tự cài đặt trong `ProductLookup.cpp`: FNV-1a, separate chaining, mở rộng bucket khi load factor vượt 0,75. `ProductService::findById` trả con trỏ const hoặc `nullptr`. ID được so sánh chính xác, phân biệt hoa/thường.

Bổ sung `HashTable::swap` để thay toàn bộ dữ liệu sau khi loader kiểm tra thành công. ProductStore thêm revision để đồng bộ chỉ mục giá. Con trỏ bản ghi vẫn giữ địa chỉ qua rehash nhưng **không còn hợp lệ sau xóa, load thay snapshot hoặc hủy store**.

### Tạo và quản lý đơn

Giữ các quy tắc đã có trong `OrderService::createOrder`:

1. Kiểm tra khách hàng, ưu tiên 1–3, đơn không rỗng và số lượng dương.
2. Gộp Product ID trùng trước khi kiểm tra tồn kho.
3. Kiểm tra mọi sản phẩm và mọi phép nhân/cộng tiền trước khi lưu đơn.
4. Lưu tên, đơn giá tại thời điểm mua; tổng tiền được core tính, không lấy tổng do giao diện cung cấp.
5. Sinh mã `ORD...` không trùng, lưu đơn rồi mới trừ kho.

Thay đổi bổ sung:

- Khởi tạo node lịch sử **PENDING** cùng thời điểm tạo đơn.
- Tăng revision của sản phẩm sau trừ kho và của đơn sau thêm đơn.
- Cùng OrderStore được sử dụng bởi tra Order ID, trạng thái và ưu tiên.
- Danh sách đơn vẫn trả bản sao và sort mới nhất trước; shared history bất biến giúp bản sao không sở hữu raw pointer trùng nhau.
- Không đổi nghiệp vụ tính tiền và không tạo đơn dở khi gặp lỗi ở mặt hàng cuối.

Ví dụ: P001 giá 500.000, tồn 20; P002 giá 750.000, tồn 10. Yêu cầu P001 ×2, P002 ×1, P001 ×3 được gộp thành P001 ×5 và P002 ×1. Tổng 3.250.000, tồn còn 15 và 9. Nếu P002 thiếu kho, cả hai tồn kho và số đơn giữ nguyên.

## 4. Phần Nam

`origin/Product` chưa có code chức năng tại thời điểm kiểm tra. `ProductManagementService` được bổ sung để bản tổng thể hoạt động, theo phạm vi đã phân công cho Nam:

- `addProduct`: tái sử dụng validation và insert sản phẩm của ProductService, chặn ID trùng và dữ liệu âm.
- `updateInfo`: cập nhật tên, giá và ngưỡng cảnh báo. Cấp phát tên mới trước khi thay đổi bản ghi. Đơn cũ giữ snapshot tên/giá.
- `changeStock`: nhập hoặc xuất số lượng dương; nhập kiểm tra overflow, xuất kiểm tra đủ kho. Số lượng tồn là lượng còn có thể bán, vì hàng đặt trong đơn đã được trừ.
- `removeProduct`: không xóa sản phẩm đã xuất hiện trong **bất kỳ đơn nào**, kể cả đơn kết thúc. Quy tắc này bảo vệ liên kết lịch sử và khả năng hoàn kho; nhóm có thể đổi sang cơ chế ẩn sản phẩm ở phiên bản sau.
- Mỗi cập nhật thành công tăng revision của ProductStore.

Nam cần tự review implementation mới và bổ sung biện minh, test/debug cá nhân; không ghi đây là code đã có trên nhánh Product từ trước.

## 5. Phần Đình Hải

### Tra cứu theo Order ID

Phân công mới xác nhận Đình Hải tra **mã đơn hàng**. File nguồn `Tracuutheoma.cpp` lại tra Product ID bằng binary search, nên không đưa thêm một chỉ mục sản phẩm trùng với phần của An.

`OrderLookupService::findById` trong `OrderLookup.cpp` dùng OrderStore chung, trả đúng đơn gồm mặt hàng, tổng tiền, trạng thái và lịch sử. Không tạo một map/danh sách đơn riêng. Hàm tra đơn cũ trong OrderService được giữ để không phá cách gọi từ phần đã có.

### Tìm sản phẩm theo khoảng giá

Giữ ý tưởng binary search hai biên nhưng sửa điều kiện đầu vào và đồng bộ:

- Tạo bản sao sản phẩm sort theo **giá tăng**, cùng giá sort theo Product ID.
- Khi revision chưa đổi thì tái sử dụng chỉ mục; khi thêm/sửa/xóa hoặc đổi kho thì làm mới.
- Không dùng chung vector sort theo mã để binary search giá.
- Lấy kích thước từ vector, bỏ tham số `quantity` dễ lệch với dữ liệu thật.
- Tìm first price >= min và first price > max; khoảng truy vấn bao gồm hai biên.
- Kiểm tra min không âm và min <= max; trả `vector<Product>` để console hoặc web dùng được.

Ví dụ sau khi sửa giá P003 xuống 750.000, truy vấn [750.000, 750.000] thấy cả P002 và P003. Không cần người dùng tự sắp dữ liệu trước.

## 6. Phần Đăng Khoa

### Xử lý ưu tiên

Code gốc dùng `std::priority_queue` và comparator đưa priority lớn hơn ra trước. Điều này ngược quy tắc nhóm: **1 cao nhất, 2 bình thường, 3 thấp**. Kiểm tra nguồn trước khi sửa cho thấy LOW priority=3 ra trước HIGH priority=1.

Sửa trong `PriorityOrders.cpp`:

- Tự cài đặt **binary min-heap**: `build`, `siftDown`, `push`, `top`, `pop`; không dùng priority_queue.
- Key sắp theo priority tăng, thời gian tạo tăng, rồi số thứ tự mã đơn tăng để kết quả xác định khi hòa.
- Heap chỉ giữ ID/priority/thời gian, không giữ một bản sao Order làm nguồn dữ liệu chính.
- `processNext` chỉ chọn đơn **PENDING** và gọi OrderStatusService để chuyển bản ghi thật sang PROCESSING, ghi lịch sử.
- Revision thay đổi do tạo/hủy/chuyển trạng thái/load khiến heap làm mới. Sau pop thành công có thể tiếp tục dùng phần heap còn lại.
- Đơn đã hủy hoặc đã xử lý không còn được chọn. Queue rỗng trả EMPTY_QUEUE thay vì throw làm dừng chương trình.

Không tuyên bố mọi lần gọi processNext đều O(log n): khi cache bị invalidated, service cần quét OrderStore và build lại heap. Đây là đánh đổi để tích hợp đúng dữ liệu trước khi tối ưu thêm.

### Cảnh báo hàng sắp hết

Code gốc so `stock / totalStock < 15%`, dùng một totalStock chung và không đọc minStock. Bản mới kiểm tra **`stock <= minStock` của từng sản phẩm**. stock=0 được giao diện đánh dấu hết hàng; không có cảnh báo nếu stock lớn hơn ngưỡng.

LowStockService trả danh sách sản phẩm thay vì in trực tiếp. Mỗi lần xem đọc kho hiện tại nên phản ánh ngay nhập/xuất, tạo đơn và hủy đơn. Độ phức tạp là O(n) quét, cộng sort theo mã cho danh sách cảnh báo.

## 7. Phần Huỳnh Khoa

Giữ transition gốc:

```text
PENDING -> PROCESSING -> SHIPPING -> COMPLETED
PENDING -> CANCELLED
PROCESSING -> CANCELLED
```

Không cho hủy đơn SHIPPING; COMPLETED/CANCELLED là trạng thái kết thúc. Không cho chuyển lại chính trạng thái hiện tại.

Thay đổi trong `OrderStatus.cpp`:

- Dùng OrderStore chung thay vì unordered_map riêng, do đó đơn vừa tạo có thể được theo dõi ngay.
- Lịch sử vẫn là chuỗi node, nhưng node bất biến và được quản lý bằng shared_ptr. Copy Order chia sẻ lịch sử bất biến an toàn; tránh leak/raw pointer và nguy cơ double-free nếu chỉ thêm destructor vào bản cũ.
- Ghi trạng thái và Unix milliseconds, bắt đầu bằng PENDING. Khi cập nhật, timestamp không nhỏ hơn sự kiện trước.
- `history` trả các sự kiện **cũ đến mới** để dễ theo dõi.
- Khi hủy, kiểm tra mọi sản phẩm và khả năng cộng kho không tràn trước; cấp phát node lịch sử trước khi thay đổi trạng thái/kho.
- Hủy thành công hoàn lượng từng mặt hàng đúng một lần. Hủy lại bị từ chối; không hoàn kho lần hai.
- Nếu thiếu bản ghi sản phẩm hoặc cộng kho bị tràn, đơn và mọi tồn kho giữ nguyên.
- Module ưu tiên gọi cùng updateStatus nên transition và lịch sử không bị bỏ qua.

## 8. Phần tích hợp bổ sung

### Console chung

Một main.cpp với các menu sản phẩm, nhập/xuất kho, tra Product ID, tạo/tra đơn, khoảng giá, xử lý ưu tiên, trạng thái, lịch sử, cảnh báo và lưu file. Menu chỉ thu thập input và hiển thị output. Giá/tổng/kho được kiểm tra trong core.

### Persistence CSV

Bổ sung FileStorage vì bản tổng thể cần giữ dữ liệu qua các lần chạy:

- Snapshot gồm VERSION, PRODUCT, ORDER, ITEM, HISTORY. Tên có dấu phẩy, nháy kép hoặc xuống dòng được escape.
- Load vào hai store tạm; kiểm tra ID trùng, số âm, số quá lớn, ưu tiên, trạng thái, mặt hàng tồn tại, tổng tiền, lịch sử và thứ tự chuyển trạng thái.
- Chỉ swap sang store đang chạy khi snapshot hợp lệ hoàn toàn; lỗi không làm mất dữ liệu đang có.
- Nạp đơn đã lưu trực tiếp, **không gọi createOrder**, tránh trừ kho hai lần.
- Save ghi file `.tmp`, đóng và kiểm tra lỗi rồi thay file đích. Nếu đã có file cũ, giữ `.bak` cho đến khi thay thành công; có bước khôi phục khi rename lỗi.
- Chặn save khi `.tmp`/`.bak` cũ còn tồn tại để người dùng kiểm tra dữ liệu phục hồi.
- Main tự nạp `data/shop.csv`, tự lưu sau thay đổi thành công; gặp snapshot lỗi lúc khởi động thì dừng, không ghi đè file lỗi. `--demo` chỉ seed khi file chưa tồn tại.

Nếu save thất bại sau nghiệp vụ thành công, dữ liệu trong bộ nhớ vẫn đã thay đổi; console báo lỗi và thử lưu lại lúc thoát. Đây là file persistence cho một tiến trình, chưa phải transaction database hoặc cơ chế chịu mất điện.

## 9. Cấu trúc dữ liệu và chi phí

| Thao tác | Chi phí và điều kiện |
|---|---|
| Tra Product ID / Order ID | O(1) kỳ vọng khi độ dài mã giới hạn; O(n) xấu nhất khi va chạm. Key dài L cần chi phí băm O(L) |
| Tạo đơn m dòng, k mã khác nhau | O(m) kỳ vọng để gộp và tra; bộ nhớ tạm O(k). Insert vào hash có thể rehash O(số đơn); sau load, lần đầu sinh ID có thể quét qua các mã đã dùng |
| Danh sách đơn | O(n log n) so sánh cộng chi phí sao chép mặt hàng |
| Khoảng giá trên cache hợp lệ | O(log n + k); làm mới chỉ mục tốn O(n log n) và bộ nhớ O(n) |
| Heap build / push / pop | Build O(p), push/pop O(log p), top O(1), p là số đơn PENDING |
| Priority service khi revision đổi | Quét n đơn và build p entry trước pop; lần liên tiếp không invalidation dùng heap còn lại |
| Chuyển trạng thái thường | Hash lookup kỳ vọng O(1), thêm node lịch sử O(1) |
| Hủy đơn k mặt hàng | O(k) kỳ vọng; prevalidate trước khi hoàn kho |
| Cảnh báo kho | Quét O(n), sort w cảnh báo O(w log w) |
| Load/save | Tuyến tính theo lượng dữ liệu và nội dung chuỗi trong trường hợp hash phân bố tốt; load cần bản snapshot tạm |

**Hash table và binary heap là hai loại cấu trúc trung tâm tự cài đặt.** Dùng hai hash store không được tính thành hai loại. Vector và sort thư viện được dùng cho lưu tuần tự/chỉ mục; nhóm cần biện minh Q1–Q4 cho từng yêu cầu và giải thích xung đột tra ID với truy vấn có thứ tự.

## 10. Kết quả kiểm chứng

Kết quả bên dưới ghi nhận lượt kiểm chứng trước khi dọn gọn. Theo yêu cầu của người dùng, source test/benchmark và CSV kết quả đã được bỏ khỏi bản hiện tại; có thể xem lại trong commit `59104b4`. Không còn target CTest hoặc benchmark trong CMake hiện tại.

Môi trường: Windows, AMD Ryzen 7 7435HS, GCC 15.2.0 MinGW UCRT64, C++17, CMake Release. Kiểm chứng ngày 03/10/2026.

- Build thành công: dsa_demo, integration_tests, dsa_benchmark.
- CTest: **1/1 executable kiểm thử pass**, chương trình thực hiện **24.130 kiểm tra**. Đây không phải 24.130 tình huống khác nhau: có vòng lặp trên 10.000 key và 2.000 heap entry.
- Hash: insert/find, key trùng, rehash, giữ địa chỉ, erase; heap build và push/pop được đối chiếu thứ tự với vector sort cho 2.000 entry ngẫu nhiên.
- Nghiệp vụ: số âm/0, mã không có, thiếu kho ở mặt hàng cuối, gộp mã trùng, overflow số lượng/tiền, giá snapshot và không xóa sản phẩm đã có đơn.
- MC2: khoảng giá bao gồm biên, không có kết quả, giá trùng và cập nhật cache; priority=1 trước 3, FIFO cùng mức, queue rỗng, bỏ đơn đã hủy.
- Trạng thái: chặn bước nhảy và hủy sau SHIPPING, lịch sử đầy đủ, copy an toàn, hủy từ PENDING/PROCESSING, hoàn kho một lần, overflow hoàn kho giữ nguyên trạng thái.
- CSV: round trip với dấu phẩy/nháy/xuống dòng, giữ lịch sử và trạng thái, không trừ kho lại, sinh ID sau load không trùng, cache được làm mới; chặn dữ liệu âm, ID trùng, CSV hỏng, đơn thiếu item/history và tổng tiền sai.
- Console hai phiên: thêm/sửa/nhập/xuất, tạo hai đơn ưu tiên 3 và 1, xử lý đơn priority=1 trước, hủy đơn còn lại, hoàn kho, hoàn thành đơn đã xử lý, xem lịch sử/cảnh báo, lưu và mở lại. Kết quả cuối P001=20, P002=9, P004=5; ORD000001 CANCELLED, ORD000002 COMPLETED; lịch sử đơn hoàn thành có 4 trạng thái.

### Benchmark 10.000 và 100.000 bản ghi

Một lượt warm-up và ba lượt đo, lấy median, đổi thứ tự đo, seed 42, **1.000 thao tác/workload**. Optimized và linear phải có checksum bằng nhau. Dữ liệu thô từng được lưu trong `KET_QUA_BENCHMARK.csv` tại commit `59104b4`.

| Workload | Bản ghi | Tối ưu median ms | Tuyến tính median ms |
|---|---:|---:|---:|
| Tra ID | 10.000 | 0,1215 | 12,3446 |
| Khoảng giá trên cache | 10.000 | 0,3556 | 9,4833 |
| Build heap và pop 1.000 lần | 10.000 | 0,3657 | 7,9322 |
| Tra ID | 100.000 | 0,2102 | 199,1200 |
| Khoảng giá trên cache | 100.000 | 0,7705 | 312,7650 |
| Build heap và pop 1.000 lần | 100.000 | 4,3756 | 380,5650 |

Giới hạn đo: tra ID có 80% hit và 20% miss; khoảng giá hẹp với ít kết quả, **không tính sort khởi tạo cache**; heap tính cả copy/build và pop. Heap benchmark đo cấu trúc trực tiếp, chưa tính updateStatus/lưu CSV trong service. Mọi thời gian là cho cả batch 1.000 thao tác, không phải một thao tác. Không đo HTTP/UI, không chứng minh worst-case và không dùng số liệu này để khẳng định tốc độ toàn hệ thống.

## 11. Cách chạy và tiếp tục làm việc

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic main.cpp -o dsa_demo.exe
.\dsa_demo.exe --demo
```

Hoặc xem README để build chương trình chính bằng CMake. `--data path.csv` chọn snapshot khác. Chỉ dùng --demo lần đầu trên file chưa tồn tại để có P001/P002/P003.

Nhóm lấy nhánh `merge`, mỗi thành viên chạy phần mình và review code trước khi mở PR vào main. Các nhánh cũ là nguồn đối chiếu; tránh tiếp tục phát triển trên model/store độc lập rồi đưa lại vào hệ thống.

## 12. Giới hạn và việc còn lại

1. Core hiện một luồng. Nếu nối API nhiều luồng, phải đồng bộ toàn bộ thao tác liên quan kho/đơn; không chỉ khóa từng find.
2. Chưa có web/API, xác thực, thanh toán hoặc phân trang. Đây là bản console tích hợp.
3. Save là snapshot toàn bộ sau mỗi thay đổi, có thể tốn thời gian ở dữ liệu lớn. Chưa có khóa nhiều tiến trình, transaction, fsync hoặc bảo đảm phục hồi sau mất điện; backup cần người dùng xử lý nếu thao tác trước bị gián đoạn.
4. Chỉ mục giá là bản sao cache; heap có thể rebuild khi revision thay đổi. Khi workload cập nhật rất dày, cần đo thêm và cân nhắc chỉ mục cập nhật tăng dần.
5. ID phân biệt hoa/thường. Số điện thoại mới kiểm tra không rỗng, chưa có quy tắc định dạng. Giá có thể bằng 0; minStock độc lập với stock hiện tại.
6. Lịch sử đơn không cho sửa/xóa trực tiếp. Sản phẩm đã được tham chiếu không được xóa; nếu cần ẩn/ngừng bán phải bổ sung trường riêng.
7. Nhóm cần hoàn thiện D2/D3 theo thiết kế cuối, Q1–Q4 và yêu cầu xung đột; mỗi người tự làm review D6, nhật ký AI/phản tư D7 và ôn D8. Mỗi thành viên cần tự kiểm chứng phần mình phụ trách và chuẩn bị bảo vệ cá nhân.
