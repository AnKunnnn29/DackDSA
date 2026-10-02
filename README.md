# DackDSA

Phần core C++17: tra cứu sản phẩm theo mã và tạo/quản lý đơn hàng.

- `Models.h`: dữ liệu sản phẩm, mặt hàng và đơn hàng dùng chung.
- `ProductLookup.h/.cpp`: bảng băm tự cài đặt và tra cứu sản phẩm. Hàm thêm/danh sách sản phẩm hỗ trợ nạp dữ liệu và chọn mặt hàng.
- `OrderManager.h/.cpp`: tạo đơn nhiều sản phẩm, kiểm tra tồn kho, tính tổng tiền, xem danh sách và chi tiết đơn.
- `main.cpp`: chương trình console chạy thử với P001, P002, P003.

## Chạy với g++

```powershell
g++ -std=c++17 -Wall -Wextra main.cpp ProductLookup.cpp OrderManager.cpp -o dsa_demo.exe
.\dsa_demo.exe
```

Hoặc dùng CMake: `cmake -S . -B build`, `cmake --build build`.

Các module của nhóm dùng chung `shop::ProductStore` và `shop::OrderStore`, truyền vào `shop::ProductService` và `shop::OrderService`. Không tạo kho/đơn hàng riêng cho từng module.

Mã phân biệt chữ hoa/thường. Tạo đơn gộp mã trùng, kiểm tra tất cả mặt hàng trước khi trừ kho, lưu tên và đơn giá tại thời điểm mua. Đơn mới ở PENDING. Dữ liệu chỉ ở bộ nhớ, mất khi thoát; core hiện dùng một luồng. Web, lưu file, xử lý ưu tiên, chuyển trạng thái và lịch sử do các phần khác tích hợp.
