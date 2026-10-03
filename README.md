# DackDSA

Bản tích hợp **quản lý kho và xử lý đơn hàng thương mại điện tử**, C++17, nhánh `merge`.

Đọc [báo cáo tích hợp](BAO_CAO_TICH_HOP.md) để xem phân công, nguồn từng module, thay đổi và giới hạn.

## Chạy với g++

```powershell
# Nếu MinGW chưa nằm trong PATH, đổi đường dẫn theo máy của bạn.
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic main.cpp -o dsa_demo.exe
.\dsa_demo.exe --demo
```

Chỉ compile `main.cpp`: nó include các module `.cpp`. Không thêm các module đó vào cùng lệnh compile, tránh trùng định nghĩa. Mỗi executable là một đơn vị biên dịch; nếu sau này muốn build các module riêng biệt thì cần tách giao diện `.h`.

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

Chưa có web/API và hỗ trợ nhiều luồng. Hash và binary heap là hai loại cấu trúc trung tâm tự cài đặt; vector/sort vẫn dùng thư viện chuẩn. Nhật ký AI, biện minh Q1–Q4, review và bảo vệ cá nhân cần mỗi thành viên tự hoàn thiện.
