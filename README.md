# Hydroponic- Hệ thống trồng rau thủy canh tự động 

This is a Hobby Project. There are few coding that I borrow from other. I am still new to all of this. So there are more room to improvement in this project. 

Đây là công việc mình đam mê. Mọi thứ vẫn còn mới mẻ với mình. Cho nên dự án này còn nhiều khoảng trống để cải thiện. 

Các bạn có thể nhìn hình Bản thiết kế của mình để hiểu thêm chi tiết. Mình dùng Uno board, ESP32 và Raspberry pi 4 trong hệ thống điều khiển này. 

Mình cài đặt Home Assistant vào Raspberry pi 4. Và nó sẽ được kết nối với wifi.  Raspberry pi 4 giống như một máy chủ (host or server). Home Assistant dùng để trao đổi dữ liệu hay điều khiển các thiết bị khác. Nó giống như hệ thống điều khiển trung tâm (control center). Và trong hệ thống của mình, Nó sẽ trao đổi thông tin và điều khiển với ESP32.

Mình cũng cài đặt mqtt vào Home Assistant (Raspberry pi 4) và ESP32. MQTT giúp cho Raspberry pi 4 và ESP32 dễ kết nối và truyền thông tin với nhau. MQTT sử dụng rất nhiều trong những thiết bị IoT.  
 
Bởi vì EPS32 không có nhiều pin, cho nên mình sẽ kết nối ESP32 với Uno. 

Đa số  thiết bị cảm biến trong hệ thống này được nối với Uno. Nhưng Uno không có chức năng kết nối wifi, cho nên nó sẽ chuyển mọi dữ liệu tới ESP32. Và ESP32 sẽ giúp nó truyền tải tới Home Assistant (Raspberry pi 4). Và mình có thể dùng Home Assistant để điều khiển các thiết bị kết nối ESP32 hay Uno. 

Có nhiều từ ngữ trong tiếng Anh mình không biết dịch ra tiếng Việt như thế nào. Cho nên mọi bình luận của mình trong coding đều bằng tiếng anh. Có thể trong tương lai, mình sẽ cố gắng dịch. 
