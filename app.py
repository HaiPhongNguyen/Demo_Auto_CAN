import sys
import serial
import serial.tools.list_ports
from datetime import datetime
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QLabel, QPushButton, QComboBox, 
                             QSlider, QGroupBox, QProgressBar, QTextEdit, QGridLayout)
from PyQt5.QtCore import QTimer, Qt

class CANBusApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("CAN Vehicle Monitor & Control - Full Sensors")
        self.setMinimumSize(1000, 800)
        
        self.serial_port = None
        self.initUI()
        
        self.timer = QTimer()
        self.timer.timeout.connect(self.read_serial)
        self.timer.start(30)

    def initUI(self):
        main_layout = QVBoxLayout()
        central_widget = QWidget()
        central_widget.setLayout(main_layout)
        self.setCentralWidget(central_widget)

        # --- 1. Top Bar: Connection ---
        conn_layout = QHBoxLayout()
        self.port_combo = QComboBox()
        self.refresh_ports()
        self.btn_connect = QPushButton("Connect")
        self.btn_connect.clicked.connect(self.toggle_connection)
        
        conn_layout.addWidget(QLabel("Port:"))
        conn_layout.addWidget(self.port_combo)
        conn_layout.addWidget(self.btn_connect)
        conn_layout.addStretch()
        main_layout.addLayout(conn_layout)

        # --- 2. Middle Layout (Dashboard & Control) ---
        mid_layout = QHBoxLayout()

        # --- Left: Dashboard (Speed, Battery, Door) ---
        dash_group = QGroupBox("Trạng thái hệ thống")
        dash_v = QVBoxLayout()
        
        self.lbl_speed = QLabel("Tốc độ: 0 km/h")
        self.lbl_speed.setStyleSheet("font-size: 22px; color: #0078d7; font-weight: bold;")
        
        self.lbl_soc = QLabel("Pin (SOC): 0%")
        self.progress_soc = QProgressBar()
        
        self.lbl_door = QLabel("CỬA: ĐANG ĐÓNG")
        self.lbl_door.setAlignment(Qt.AlignCenter)
        self.lbl_door.setStyleSheet("background-color: green; color: white; font-weight: bold; padding: 10px; border-radius: 5px;")
        
        self.lbl_rfid = QLabel("RFID UID: ----")
        
        dash_v.addWidget(self.lbl_speed)
        dash_v.addWidget(self.lbl_soc)
        dash_v.addWidget(self.progress_soc)
        dash_v.addSpacing(10)
        dash_v.addWidget(self.lbl_door)
        dash_v.addWidget(self.lbl_rfid)
        dash_group.setLayout(dash_v)

        # --- Center: Obstacle Sensors (Siêu âm) ---
        obs_group = QGroupBox("Cảm biến vật cản (cm)")
        obs_grid = QGridLayout()
        
        # Tạo các thanh bar hiển thị khoảng cách
        self.bar_left = QProgressBar()
        self.bar_right = QProgressBar()
        self.bar_rear = QProgressBar()
        for b in [self.bar_left, self.bar_right, self.bar_rear]:
            b.setRange(0, 400) # Giả định tối đa 400cm
            b.setTextVisible(True)

        obs_grid.addWidget(QLabel("Bên Trái:"), 0, 0)
        obs_grid.addWidget(self.bar_left, 0, 1)
        obs_grid.addWidget(QLabel("Bên Phải:"), 1, 0)
        obs_grid.addWidget(self.bar_right, 1, 1)
        obs_grid.addWidget(QLabel("Phía Sau:"), 2, 0)
        obs_grid.addWidget(self.bar_rear, 2, 1)
        
        self.lbl_buzzer = QLabel("BUZZER: OFF")
        self.lbl_buzzer.setAlignment(Qt.AlignCenter)
        self.lbl_buzzer.setStyleSheet("background-color: #ddd; padding: 5px;")
        obs_grid.addWidget(self.lbl_buzzer, 3, 0, 1, 2)
        
        obs_group.setLayout(obs_grid)

        # --- Right: Control Panel ---
        ctrl_group = QGroupBox("Điều khiển")
        ctrl_v = QVBoxLayout()
        self.speed_slider = QSlider(Qt.Horizontal)
        self.speed_slider.setRange(0, 255)
        self.lbl_speed_cmd = QLabel("Lệnh tốc độ: 0")
        self.speed_slider.valueChanged.connect(lambda v: self.lbl_speed_cmd.setText(f"Lệnh tốc độ: {v}"))
        
        btn_layout = QGridLayout()
        self.btn_fwd = QPushButton("THUẬN")
        self.btn_rev = QPushButton("NGƯỢC")
        self.btn_stop = QPushButton("STOP")
        self.btn_stop.setStyleSheet("background-color: #ff4444; color: white; font-weight: bold;")
        
        btn_layout.addWidget(self.btn_fwd, 0, 0)
        btn_layout.addWidget(self.btn_rev, 0, 1)
        btn_layout.addWidget(self.btn_stop, 1, 0, 1, 2)
        
        # Gán sự kiện
        self.btn_fwd.clicked.connect(lambda: self.send_command("FWD"))
        self.btn_rev.clicked.connect(lambda: self.send_command("REV"))
        self.btn_stop.clicked.connect(lambda: self.send_command("STOP"))

        ctrl_v.addWidget(self.lbl_speed_cmd)
        ctrl_v.addWidget(self.speed_slider)
        ctrl_v.addLayout(btn_layout)
        ctrl_group.setLayout(ctrl_v)

        # Gom 3 cột vào mid_layout
        mid_layout.addWidget(dash_group, 1)
        mid_layout.addWidget(obs_group, 1)
        mid_layout.addWidget(ctrl_group, 1)
        main_layout.addLayout(mid_layout)

        # --- 3. Log Screen ---
        self.log_screen = QTextEdit()
        self.log_screen.setReadOnly(True)
        self.log_screen.setStyleSheet("background-color: #1e1e1e; color: #00ff00; font-family: Consolas;")
        main_layout.addWidget(QLabel("Log Frame (Gửi/Nhận):"))
        main_layout.addWidget(self.log_screen, 2)

    # --- Logic xử lý Serial & CAN ---
    def refresh_ports(self):
        self.port_combo.clear()
        self.port_combo.addItems([p.device for p in serial.tools.list_ports.comports()])

    def toggle_connection(self):
        if self.serial_port is None or not self.serial_port.is_open:
            try:
                self.serial_port = serial.Serial(self.port_combo.currentText(), 115200, timeout=0.01)
                self.btn_connect.setText("Disconnect")
                self.write_log("SYSTEM", "Connected.")
            except Exception as e:
                self.write_log("ERROR", str(e))
        else:
            self.serial_port.close()
            self.btn_connect.setText("Connect")
            self.write_log("SYSTEM", "Disconnected.")

    def write_log(self, direction, message):
        t = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        color = "#00ff00" if direction == "RX" else "#ffaa00" if direction == "TX" else "#ffffff"
        self.log_screen.append(f"<span style='color:gray'>[{t}]</span> <span style='color:{color}'><b>{direction}:</b> {message}</span>")
        self.log_screen.moveCursor(self.log_screen.textCursor().End)

    def read_serial(self):
        if self.serial_port and self.serial_port.is_open:
            if self.serial_port.in_waiting > 0:
                try:
                    line = self.serial_port.readline().decode('utf-8').strip()
                    if line:
                        self.write_log("RX", line)
                        self.parse_can_data(line)
                except: pass

    def parse_can_data(self, data):
        try:
            p = data.split(',')
            can_id = p[0]
            # 0x101: Speed
            if can_id == "101":
                val = (int(p[1]) << 8) | int(p[2])
                self.lbl_speed.setText(f"Tốc độ: {val} km/h")
            # 0x201, 202, 203: Obstacles
            elif can_id == "201":
                val = (int(p[1]) << 8) | int(p[2])
                self.bar_left.setValue(min(val, 400))
            elif can_id == "202":
                val = (int(p[1]) << 8) | int(p[2])
                self.bar_right.setValue(min(val, 400))
            elif can_id == "203":
                val = (int(p[1]) << 8) | int(p[2])
                self.bar_rear.setValue(min(val, 400))
            # 0x204: Buzzer
            elif can_id == "204":
                if int(p[3]) == 1:
                    self.lbl_buzzer.setText("BUZZER: ON (CẢNH BÁO!)")
                    self.lbl_buzzer.setStyleSheet("background-color: red; color: white; font-weight: bold;")
                else:
                    self.lbl_buzzer.setText("BUZZER: OFF")
                    self.lbl_buzzer.setStyleSheet("background-color: #ddd;")
            # 0x301: RFID UID
            elif can_id == "301":
                uid = "-".join(p[1:5])
                self.lbl_rfid.setText(f"RFID UID: {uid.upper()}")
            # 0x302: Door
            elif can_id == "302":
                if int(p[5]) == 1:
                    self.lbl_door.setText("CỬA: ĐANG MỞ")
                    self.lbl_door.setStyleSheet("background-color: red; color: white; font-weight: bold; padding: 10px;")
                else:
                    self.lbl_door.setText("CỬA: ĐANG ĐÓNG")
                    self.lbl_door.setStyleSheet("background-color: green; color: white; font-weight: bold; padding: 10px;")
            # 0x404: SOC
            elif can_id == "404":
                soc = int(p[6])
                self.progress_soc.setValue(soc)
                self.lbl_soc.setText(f"Pin (SOC): {soc}%")
        except: pass

    def send_command(self, action):
        if self.serial_port and self.serial_port.is_open:
            speed = self.speed_slider.value()
            cmd = f"CMD,{action},{speed}\n"
            self.serial_port.write(cmd.encode())
            self.write_log("TX", cmd.strip())

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = CANBusApp()
    window.show()
    sys.exit(app.exec_())