đường dẫn thư mục chứa folder bộ code
1* cd F:\...\xiaozhi-esp32
2*/ idf.py set-target esp32s3
3*/ idf.py menuconfig
trong menu có thể tùy chọn loại màn hình đang dùng và key word( từ dùng đánh thức thiết bị)
4*/ idf.py build
5*/ idf.py merge-bin ( gộp thành 1 file.bin
6*/ idf.py -p COMxx -b 921600 build flash monitor  # úp fw vào esp32s3  cổng com tra trên web hoặc trong máy)