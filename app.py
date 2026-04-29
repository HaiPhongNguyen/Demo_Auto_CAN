import sys
import serial
import serial.tools.list_ports
from datetime import datetime
import struct

from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QLabel, QPushButton, QComboBox, 
                             QSlider, QGroupBox, QProgressBar, QTextEdit, QGridLayout)
from PyQt5.QtCore import QTimer, Qt
from PyQt5.QtGui import QPixmap, QFont


class CANBusApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("IUH - CAN BUS MONITORING SYSTEM")
        self.setMinimumSize(1200, 900)
        
        self.serial_port = None
        self.initUI()
        
        # Timer quét dữ liệu Serial
        self.timer = QTimer()
        self.timer.timeout.connect(self.read_serial)
        self.timer.start(30)

    def initUI(self):
        main_layout = QVBoxLayout()
        central_widget = QWidget()
        central_widget.setLayout(main_layout)
        self.setCentralWidget(central_widget)

        # --- PHẦN 1: HEADER (LOGO TRÁI - TÊN TRƯỜNG DÀI - TÊN KHOA CĂN GIỮA) ---
        header_outer_layout = QVBoxLayout()
        header_outer_layout.setContentsMargins(20, 10, 20, 10)
        
        # Hàng ngang chính chứa [Logo] và [Khối chữ]
        top_row_content = QHBoxLayout()
        top_row_content.setAlignment(Qt.AlignCenter)

        # 1. Logo (Giữ nguyên kích thước 300x90)
        self.lbl_logo = QLabel()
        pixmap = QPixmap("logo.png")
        if not pixmap.isNull():
            self.lbl_logo.setPixmap(pixmap.scaled(300, 90, Qt.KeepAspectRatio, Qt.SmoothTransformation))
        else:
            self.lbl_logo.setText("IUH LOGO")
            self.lbl_logo.setFixedSize(300, 90)
            self.lbl_logo.setStyleSheet("border: 1px dashed #ccc; background: #f9f9f9; color: #b71c1c;")
        self.lbl_logo.setAlignment(Qt.AlignRight | Qt.AlignVCenter)

        # 2. Khối chữ (Tên trường dài, Tên khoa căn giữa theo tên trường)
        text_block_layout = QVBoxLayout()
        text_block_layout.setSpacing(2) # Khoảng cách hẹp giữa 2 dòng chữ
        
        # Tên trường: Kéo dài ra, không cho xuống dòng
        school_name = QLabel("ĐẠI HỌC CÔNG NGHIỆP THÀNH PHỐ HỒ CHÍ MINH")
        school_name.setStyleSheet("font-size: 16pt; font-weight: bold; color: #b71c1c;")
        school_name.setWordWrap(False) 
        school_name.setAlignment(Qt.AlignLeft) 
        
        # Tên khoa: Căn giữa theo chiều ngang của khối chữ (tức là căn giữa theo tên trường)
        faculty_name = QLabel("KHOA ĐIỆN TỬ VIỄN THÔNG")
        faculty_name.setStyleSheet("font-size: 14pt; font-weight: bold; color: #0D47A1;")
        faculty_name.setAlignment(Qt.AlignCenter) 

        text_block_layout.addWidget(school_name)
        text_block_layout.addWidget(faculty_name)

        # Thêm vào hàng ngang (Không dùng stretch để các thành phần đứng sát nhau tự nhiên)
        top_row_content.addWidget(self.lbl_logo)
        top_row_content.addSpacing(25) # Khoảng cách giữa logo và chữ
        top_row_content.addLayout(text_block_layout)

        # 3. Thông tin sinh viên
        student_info = QLabel(
            "Sinh viên thực hiện: Đỗ Xuân Nguyên (21116421) | Trần Thế Bảo Ngọc (21139521)"
        )
        student_info.setStyleSheet("font-size: 12pt; font-weight: 500; color: #333; margin-top: 10px;")
        student_info.setAlignment(Qt.AlignCenter)

        header_outer_layout.addLayout(top_row_content)
        header_outer_layout.addWidget(student_info)
        
        main_layout.addLayout(header_outer_layout)
        main_layout.addSpacing(10)
        main_layout.addWidget(QLabel("<hr style='border: 1px solid #bbb;'>"))

        # --- PHẦN 2: KẾT NỐI SERIAL ---
        conn_layout = QHBoxLayout()
        conn_layout.addStretch()
        self.port_combo = QComboBox()
        self.port_combo.setMinimumWidth(200)
        self.refresh_ports()
        self.btn_connect = QPushButton("KẾT NỐI SERIAL")
        self.btn_connect.setFixedWidth(150)
        self.btn_connect.clicked.connect(self.toggle_connection)
        
        conn_layout.addWidget(QLabel("<b>Cổng COM:</b>"))
        conn_layout.addWidget(self.port_combo)
        conn_layout.addWidget(self.btn_connect)
        conn_layout.addStretch()
        main_layout.addLayout(conn_layout)

        # --- PHẦN 3: DASHBOARD ---
        mid_layout = QHBoxLayout()

        # Group: Thông số xe
        dash_group = QGroupBox("TRẠNG THÁI VẬN HÀNH")
        dash_v = QVBoxLayout()
        self.lbl_speed = QLabel("Tốc độ: 0 km/h")
        self.lbl_speed.setStyleSheet("font-size: 22px; color: #0D47A1; font-weight: bold;")
        self.lbl_soc = QLabel("Pin (SOC): 0%")
        self.progress_soc = QProgressBar()
        self.lbl_door = QLabel("CỬA: ĐANG ĐÓNG")
        self.lbl_door.setAlignment(Qt.AlignCenter)
        self.lbl_door.setStyleSheet("background-color: #2E7D32; color: white; font-weight: bold; padding: 15px; border-radius: 5px; font-size: 14pt;")
        self.lbl_rfid = QLabel("Mã thẻ RFID: ----")
        dash_v.addWidget(self.lbl_speed)
        dash_v.addWidget(self.lbl_soc)
        # ===== BATTERY  =====
        dash_v.addWidget(self.progress_soc)
        self.lbl_voltage = QLabel("Voltage: 0 V")
        self.lbl_current = QLabel("Current: 0 A")
        self.lbl_temp = QLabel("Temperature: 0 °C")

        for lbl in [self.lbl_voltage, self.lbl_current, self.lbl_temp]:
            lbl.setStyleSheet("font-size: 13pt; color: #1B5E20; font-weight: 600;")

        dash_v.addWidget(self.lbl_voltage)
        dash_v.addWidget(self.lbl_current)
        dash_v.addWidget(self.lbl_temp)
        dash_v.addSpacing(15)
        dash_v.addWidget(self.lbl_door)
        dash_v.addWidget(self.lbl_rfid)
        dash_group.setLayout(dash_v)

        # Group: Khoảng cách vật cản
        obs_group = QGroupBox("CẢM BIẾN VẬT CẢN (CM)")
        obs_grid = QGridLayout()
        self.bar_left = QProgressBar()
        self.bar_right = QProgressBar()
        self.bar_rear = QProgressBar()
        for b in [self.bar_left, self.bar_right, self.bar_rear]:
            b.setRange(0, 400)
            b.setTextVisible(True)
        obs_grid.addWidget(QLabel("Bên Trái:"), 0, 0)
        obs_grid.addWidget(self.bar_left, 0, 1)
        obs_grid.addWidget(QLabel("Bên Phải:"), 1, 0)
        obs_grid.addWidget(self.bar_right, 1, 1)
        obs_grid.addWidget(QLabel("Phía Sau:"), 2, 0)
        obs_grid.addWidget(self.bar_rear, 2, 1)
        self.lbl_buzzer = QLabel("BUZZER: OFF")
        self.lbl_buzzer.setAlignment(Qt.AlignCenter)
        self.lbl_buzzer.setStyleSheet("background-color: #eee; padding: 10px; border: 1px solid #ccc;")
        obs_grid.addWidget(self.lbl_buzzer, 3, 0, 1, 2)
        obs_group.setLayout(obs_grid)

        # Group: Điều khiển
        ctrl_group = QGroupBox("ĐIỀU KHIỂN")
        ctrl_v = QVBoxLayout()
        self.speed_slider = QSlider(Qt.Horizontal)
        self.speed_slider.setRange(0, 255)
        self.lbl_speed_cmd = QLabel("Tốc độ cài đặt: 0")
        self.speed_slider.valueChanged.connect(lambda v: self.lbl_speed_cmd.setText(f"Tốc độ cài đặt: {v}"))
        btn_grid = QGridLayout()
        self.btn_fwd = QPushButton("QUAY THUẬN")
        self.btn_rev = QPushButton("QUAY NGƯỢC")
        self.btn_stop = QPushButton("STOP")
        self.btn_stop.setStyleSheet("background-color: #C62828; color: white; font-weight: bold; min-height: 40px;")
        btn_grid.addWidget(self.btn_fwd, 0, 0)
        btn_grid.addWidget(self.btn_rev, 0, 1)
        btn_grid.addWidget(self.btn_stop, 1, 0, 1, 2)
        self.btn_fwd.clicked.connect(lambda: self.send_command("FWD"))
        self.btn_rev.clicked.connect(lambda: self.send_command("REV"))
        self.btn_stop.clicked.connect(lambda: self.send_command("STOP"))
        ctrl_v.addWidget(self.lbl_speed_cmd)
        ctrl_v.addWidget(self.speed_slider)
        ctrl_v.addLayout(btn_grid)
        ctrl_group.setLayout(ctrl_v)

        mid_layout.addWidget(dash_group, 1)
        mid_layout.addWidget(obs_group, 1)
        mid_layout.addWidget(ctrl_group, 1)
        main_layout.addLayout(mid_layout)

        main_layout.addWidget(QLabel("<b>LOG DỮ LIỆU CAN BUS:</b>"))
        self.log_screen = QTextEdit()
        self.log_screen.setReadOnly(True)
        self.log_screen.setStyleSheet("background-color: #1e1e1e; color: #00ff00; font-family: 'Consolas';")
        main_layout.addWidget(self.log_screen, 2)

    # ================= LOGIC =================

    def decode_float(self, p):
        raw_bytes = bytes([int(x) for x in p[1:5]])
        return struct.unpack('<f', raw_bytes)[0]

    def refresh_ports(self):
        self.port_combo.clear()
        self.port_combo.addItems([p.device for p in serial.tools.list_ports.comports()])

    def toggle_connection(self):
        if self.serial_port is None or not self.serial_port.is_open:
            try:
                self.serial_port = serial.Serial(self.port_combo.currentText(), 115200, timeout=0.01)
                self.btn_connect.setText("NGẮT KẾT NỐI")
                self.write_log("SYSTEM", f"Đã kết nối {self.port_combo.currentText()}")
            except Exception as e:
                self.write_log("ERROR", str(e))
        else:
            self.serial_port.close()
            self.btn_connect.setText("KẾT NỐI SERIAL")
            self.write_log("SYSTEM", "Đã ngắt kết nối.")

    def write_log(self, direction, message):
        t = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        color = "#00ff00" if direction == "RX" else "#ffaa00" if direction == "TX" else "#ffffff"
        self.log_screen.append(f"<span style='color:gray'>[{t}]</span> <span style='color:{color}'><b>{direction}:</b> {message}</span>")
        self.log_screen.moveCursor(self.log_screen.textCursor().End)
    
    def send_command(self, direction):
        if not (self.serial_port and self.serial_port.is_open):
            self.write_log("ERROR", "Chưa kết nối Serial")
            return

        speed = self.speed_slider.value()

        # ===== encode direction =====
        if direction == "FWD":
            dir_byte = 1
        elif direction == "REV":
            dir_byte = 2
        else:
            dir_byte = 0
            speed = 0

        # ===== CAN FRAME =====
        can_id = 0x105
        dlc = 2
        data = [dir_byte, speed]

        frame = bytes([
            (can_id >> 8) & 0xFF,
            can_id & 0xFF,
            dlc,
            dir_byte,
            speed
        ])

        self.serial_port.write(frame)
            
    def read_serial(self):
        if not (self.serial_port and self.serial_port.is_open):
            return

        try:
            data = self.serial_port.read_all()  # đọc hết sạch
            if not data:
                return

            text = data.decode('utf-8', errors='ignore')
            lines = text.splitlines()

            for line in lines:
                if line:
                    self.write_log("RX", line)
                    self.parse_can_data(line)

        except Exception as e:
            self.write_log("ERROR", str(e))

    def parse_can_data(self, data):
        try:
            if not data or not data[0].isdigit():
                return

            p = data.split(',')
            can_id = int(p[0])
                
            # RFID
            if can_id == 0x301:
                self.lbl_rfid.setText(f"Mã thẻ RFID: {' '.join(p[1:])}")
                
            # DOOR
            elif can_id == 0x302:
                is_open = int(p[1]) == 1

                self.lbl_door.setText(f"CỬA: {'ĐANG MỞ' if is_open else 'ĐANG ĐÓNG'}")
                self.lbl_door.setStyleSheet(
                    f"background-color: {'#D32F2F' if is_open else '#2E7D32'}; color: white; font-weight: bold; padding: 15px; border-radius: 5px;"
                )
                
            # SENSOR FLOAT (cm)
            elif can_id == 0x201:
                value = self.decode_float(p)
                value = max(0, min(value, 400))
                self.bar_left.setValue(int(value))
                self.bar_left.setFormat(f"{value:.1f} cm")

            elif can_id == 0x202:
                value = self.decode_float(p)
                value = max(0, min(value, 400))
                self.bar_right.setValue(int(value))
                self.bar_right.setFormat(f"{value:.1f} cm")

            elif can_id == 0x203:
                value = self.decode_float(p)
                value = max(0, min(value, 400))
                self.bar_rear.setValue(int(value))
                self.bar_rear.setFormat(f"{value:.1f} cm")

            # BUZZER
            elif can_id == 0x204:
                is_on = int(p[1]) == 1

                self.lbl_buzzer.setText(f"BUZZER: {'ON' if is_on else 'OFF'}")
                self.lbl_buzzer.setStyleSheet(
                    f"background-color: {'#D32F2F' if is_on else '#eee'}; color: {'white' if is_on else 'black'}; padding: 10px;"
                )
            # ===== BATTERY =====
            elif can_id == 0x401:
                value = self.decode_float(p)
                self.lbl_voltage.setText(f"Voltage: {value:.2f} V")

            elif can_id == 0x402:
                value = self.decode_float(p)
                self.lbl_current.setText(f"Current: {value:.2f} A")

            elif can_id == 0x403:
                value = self.decode_float(p)
                self.lbl_temp.setText(f"Temperature: {value:.2f} °C")
            # SOC
            elif can_id == 0x404:
                value = self.decode_float(p)
                self.progress_soc.setValue(int(value))
                self.lbl_soc.setText(f"Pin (SOC): {value:.1f}%")

        except Exception as e:
            self.write_log("ERROR", str(e))


if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setFont(QFont("Segoe UI", 10))
    window = CANBusApp()
    window.show()
    sys.exit(app.exec_())