# DackDSA

Phần core C++17: tra cứu sản phẩm theo mã và tạo/quản lý đơn hàng.

- `Models.h`: dữ liệu sản phẩm, mặt hàng và đơn hàng dùng chung.
- `ProductLookup.cpp`: bảng băm tự cài đặt và tra cứu sản phẩm. Hàm thêm/danh sách sản phẩm hỗ trợ nạp dữ liệu và chọn mặt hàng.
- `OrderManager.cpp`: tạo đơn nhiều sản phẩm, kiểm tra tồn kho, tính tổng tiền, xem danh sách và chi tiết đơn.
- `main.cpp`: chương trình console chạy thử với P001, P002, P003.

## Chạy với g++

```powershell
g++ -std=c++17 -Wall -Wextra main.cpp -o dsa_demo.exe
.\dsa_demo.exe
```

**Cách gộp:** `main.cpp` include `OrderManager.cpp`, và `OrderManager.cpp` include `ProductLookup.cpp`. Chỉ biên dịch `main.cpp`; không thêm hai file chức năng vào lệnh build, vì sẽ trùng định nghĩa. Các file chức năng có include guard để tránh nạp lại trong cùng một đơn vị biên dịch. Khi tích hợp nhiều file biên dịch độc lập, nhóm nên tách lại giao diện `.h`.

Hoặc dùng CMake: `cmake -S . -B build`, `cmake --build build`.

Các module của nhóm dùng chung `shop::ProductStore` và `shop::OrderStore`, truyền vào `shop::ProductService` và `shop::OrderService`. Không tạo kho/đơn hàng riêng cho từng module.

Mã phân biệt chữ hoa/thường. Tạo đơn gộp mã trùng, kiểm tra tất cả mặt hàng trước khi trừ kho, lưu tên và đơn giá tại thời điểm mua. Đơn mới ở PENDING. Dữ liệu chỉ ở bộ nhớ, mất khi thoát; core hiện dùng một luồng. Web, lưu file, xử lý ưu tiên, chuyển trạng thái và lịch sử do các phần khác tích hợp.
