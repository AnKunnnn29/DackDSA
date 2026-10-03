# Dữ liệu mẫu quản lý kho và đơn hàng

`shop.csv` là snapshot giả lập, không chứa dữ liệu khách hàng thật. Seed sinh dữ liệu: `20261003`.

| Nội dung | Số lượng |
|---|---:|
| Sản phẩm | 1.000 |
| Đơn hàng | 2.000 |
| Dòng mặt hàng | 5.954 |
| Sản phẩm có stock <= minStock | 228 |
| PENDING / PROCESSING / SHIPPING / COMPLETED / CANCELLED | 400 mỗi trạng thái |
| Ưu tiên 1 / 2 / 3 | 670 / 665 / 665 |

- Product ID: `P001` đến `P1000`; Order ID: `ORD000001` đến `ORD002000`.
- Giá đa dạng theo 20 nhóm thiết bị/phụ kiện; tồn kho gồm hết hàng, thấp hơn ngưỡng, bằng ngưỡng và an toàn.
- Mỗi đơn có 1–5 sản phẩm khác nhau, số lượng 1–4; tổng tiền bằng tổng thành tiền.
- Lịch sử bắt đầu PENDING và kết thúc đúng trạng thái hiện tại; có đơn hủy từ PENDING và PROCESSING.
- Một số đơn có đơn giá lịch sử khác giá hiện tại để minh họa snapshot giá.
- Tồn kho là lượng hiện còn có thể bán. Nạp snapshot không trừ kho lại theo các đơn cũ.

## Dùng bản mẫu mà không sửa file đã commit

Trong thư mục repository, tạo dữ liệu làm việc nếu chưa có:

```powershell
New-Item -ItemType Directory -Path data -Force | Out-Null
if (-not (Test-Path -LiteralPath 'data/shop.csv')) {
    Copy-Item -LiteralPath 'sample-data/shop.csv' -Destination 'data/shop.csv'
}
.\dsa_demo.exe
```

`data/shop.csv` là bản làm việc được Git bỏ qua; chương trình tự lưu thay đổi vào đó. `sample-data/shop.csv` là bản gốc dùng chung được commit. Không chạy chương trình với `--data sample-data/shop.csv` nếu muốn giữ nguyên bản mẫu.

Các mã để demo: `P001`, `P002`, `P003`, `P020` (hết hàng), `P010` (bằng ngưỡng), `ORD000001` (PENDING), `ORD000007` (COMPLETED), `ORD002000` (CANCELLED). Nhóm có thể reset bằng cách sao chép bản mẫu sau khi đã bảo quản dữ liệu làm việc cần giữ.
