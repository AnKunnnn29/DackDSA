# DackDSA

Bản tích hợp **quản lý kho và xử lý đơn hàng thương mại điện tử**, C++17, nhánh `merge`.

Đọc [báo cáo tích hợp](BAO_CAO_TICH_HOP.md) để xem phân công, nguồn từng module, thay đổi và giới hạn.

## Giao diện web

Giao diện HTML/CSS và JavaScript thuần, hỗ trợ máy tính và điện thoại. Để thao tác với core C++ và CSV, chạy server từ thư mục gốc repo:

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
g++ -std=c++17 -O2 server.cpp -o dsa_server.exe -lws2_32 -pthread
# Chỉ sao chép mẫu nếu chưa có dữ liệu làm việc.
New-Item -ItemType Directory -Force data | Out-Null
if (!(Test-Path data/shop.csv)) { Copy-Item sample-data/shop.csv data/shop.csv }
.\dsa_server.exe
```

Sau đó mở **http://127.0.0.1:8080** bằng Chrome hoặc Edge. Giữ server đang chạy; Ctrl+C để dừng. Với CMake, build theo mục bên dưới rồi chạy `.\build\dsa_server.exe`. Visual Studio dùng `.\build\Release\dsa_server.exe`.

Có thể dùng `--data duong-dan.csv`, `--port 8081`, `--web duong-dan-thu-muc-web`. Server yêu cầu CSV hợp lệ có sẵn và dừng nếu nạp lỗi, không tự ghi đè dữ liệu. Chạy từ thư mục gốc để đường dẫn mặc định đúng.

- Tổng quan số lượng sản phẩm, đơn hàng, cảnh báo kho và giá trị đơn hoàn tất.
- Tra sản phẩm theo mã/tên, lọc khoảng giá và tồn kho; xem chi tiết sản phẩm.
- Tra đơn theo mã/khách hàng, lọc trạng thái và mức ưu tiên; xem mặt hàng và lịch sử.
- Tạo đơn qua `OrderService` của An: kiểm tra số lượng/tồn kho, gộp sản phẩm trùng, tính tổng và lưu CSV.
- Trong chi tiết đơn: Chờ xử lý → Đang xử lý → Đang giao → Hoàn tất, qua `OrderStatusService` của Huỳnh Khoa.
- Hủy từ Chờ xử lý/Đang xử lý, có xác nhận và hoàn kho đúng một lần; lịch sử được cập nhật và lưu CSV.

Mở [web/index.html](web/index.html) trực tiếp vẫn xem được dữ liệu mẫu từ `web/data.js`, nhưng các thao tác ghi bị tắt. Khi mở qua server, web đọc CSV qua API, không dùng localStorage và không đưa đơn thử của bản cũ vào CSV. Thông tin khách hàng trong mẫu là dữ liệu giả để trình diễn.

API hiện có `GET /api/data`, `POST /api/orders`, `POST /api/orders/{id}/status`. Chuyển trạng thái gửi `{ "status": "PROCESSING", "expectedStatus": "PENDING" }`; server từ chối dữ liệu trạng thái đã cũ hoặc bước chuyển không hợp lệ. Bộ lọc danh sách/phân trang hiện xử lý trong trình duyệt; xử lý heap ưu tiên và quản lý thông tin sản phẩm vẫn dùng console.

Server chỉ nghe trên máy này (`127.0.0.1`), chưa có đăng nhập. Các yêu cầu API được khóa khi truy cập core; thao tác ghi chạy trên store tạm, lưu CSV thành công mới thay store đang chạy. **Không chạy console hoặc một server khác cùng ghi file CSV khi server web đang chạy**, vì chưa có khóa giữa các tiến trình. Hai thư viện HTTP/JSON đã lưu trong [third_party](third_party/README.md), không cần tải khi build.

## Chạy với g++

```powershell
# Nếu MinGW chưa nằm trong PATH, đổi đường dẫn theo máy của bạn.
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic main.cpp -o dsa_demo.exe
.\dsa_demo.exe --demo
```

Với console, chỉ compile `main.cpp`; với web server, chỉ compile `server.cpp`. Mỗi file include các module `.cpp`; không thêm các module đó vào cùng lệnh compile, tránh trùng định nghĩa. Mỗi executable là một đơn vị biên dịch; nếu sau này muốn build các module riêng biệt thì cần tách giao diện `.h`.

## Chạy bằng CMake

```powershell
cmake -S . -B build -G 'MinGW Makefiles' -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
.\build\dsa_demo.exe --demo
```

Với Visual Studio, dùng generator mặc định và `cmake --build build --config Release`; executable nằm ở `build/Release`.

## Menu chức năng

| Menu | Chức năng |
|---|---|
| 1–2 | Danh sách sản phẩm và tra Product ID |
| 3–6 | Thêm/sửa sản phẩm, nhập/xuất kho, xóa sản phẩm |
| 7–9 | Tạo đơn, danh sách đơn, tra Order ID |
| 10 | Truy vấn khoảng giá bao gồm hai biên |
| 11 | Chọn đơn PENDING ưu tiên cao nhất và chuyển PROCESSING |
| 12–13 | Chuyển trạng thái/hủy đơn và xem lịch sử |
| 14–15 | Cảnh báo tồn kho và lưu CSV |
| 0 | Thoát |

## Dữ liệu

Bản mẫu dùng chung gồm **1.000 sản phẩm và 2.000 đơn hàng** nằm trong [sample-data](sample-data/README.md). Sao chép `sample-data/shop.csv` vào `data/shop.csv` nếu chưa có dữ liệu làm việc; sau đó chạy chương trình bình thường. Bản mẫu có đủ ưu tiên, trạng thái, khoảng giá và cảnh báo tồn kho để demo các chức năng nhóm.

- Mặc định dùng `data/shop.csv`, tự nạp khi khởi động, tự lưu sau thao tác thay đổi thành công.
- Dùng `--data duong-dan.csv` để chọn file khác.
- `--demo` chỉ tạo P001/P002/P003 khi file chưa tồn tại; không ghi đè dữ liệu cũ.
- CSV có PRODUCT, ORDER, ITEM, HISTORY; tên chứa dấu phẩy, dấu nháy và xuống dòng được escape.
- Load kiểm tra dữ liệu trong store tạm, chỉ thay store hiện tại khi toàn bộ snapshot hợp lệ. Không tạo lại đơn và không trừ kho lần nữa.
- Save dùng `.tmp` và `.bak`; nếu file tạm/backup còn tồn tại thì cần kiểm tra trước khi lưu tiếp. Đây không phải cơ chế chịu mất điện hoặc nhiều tiến trình ghi đồng thời.
- `data/`, `.exe`, `.tmp`, `.bak` không được commit.

## Quy tắc dùng chung

- Tiền VNĐ và số lượng là số nguyên 64-bit; ID phân biệt hoa/thường.
- Ưu tiên **1 cao, 2 bình thường, 3 thấp**; cùng mức thì đơn cũ trước, cùng thời điểm thì mã sinh trước.
- Cảnh báo khi `stock <= minStock`, bao gồm hết hàng.
- Đơn mới PENDING; luồng PENDING → PROCESSING → SHIPPING → COMPLETED; chỉ hủy từ PENDING/PROCESSING.
- Tạo đơn trừ kho sau khi mọi mặt hàng hợp lệ; hủy hoàn kho đúng một lần. Đơn giữ tên và đơn giá tại thời điểm mua.
- Không xóa sản phẩm đã được ghi trong bất kỳ đơn nào; đổi tên/giá sản phẩm không làm đổi đơn cũ.
- Các service dùng chung ProductStore/OrderStore. Khi thêm module, thay đổi dữ liệu qua service để revision làm mới chỉ mục giá/heap.

## File và phạm vi

| File | Phần phụ trách |
|---|---|
| Models.h | Model, trạng thái và node lịch sử dùng chung |
| ProductLookup.cpp | Bảng băm tự cài đặt, tra cứu sản phẩm theo mã |
| OrderManager.cpp | Tạo đơn, tính tiền và danh sách đơn |
| ProductManagement.cpp | Quản lý thông tin sản phẩm và nhập/xuất kho |
| OrderLookup.cpp | Tra cứu đơn theo mã |
| PriceSearch.cpp | Chỉ mục theo giá và binary search |
| PriorityOrders.cpp | Min-heap tự cài đặt và xử lý đơn ưu tiên |
| LowStock.cpp | Cảnh báo tồn kho |
| OrderStatus.cpp | Chuyển trạng thái, hoàn kho và lịch sử |
| FileStorage.cpp | Lưu/đọc snapshot CSV |
| main.cpp | Giao diện console tổng thể |
| server.cpp | Cầu nối HTTP API, tạo đơn và chuyển trạng thái qua core, lưu CSV |
| web/ | Giao diện HTML/CSS/JavaScript |

Có HTTP API cho nạp dữ liệu, tạo đơn và chuyển trạng thái; server khóa các thao tác core bằng mutex. Hash và binary heap là hai loại cấu trúc trung tâm tự cài đặt; vector/sort vẫn dùng thư viện chuẩn. Nhật ký AI, biện minh Q1–Q4, review và bảo vệ cá nhân cần mỗi thành viên tự hoàn thiện.
