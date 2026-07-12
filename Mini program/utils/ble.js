// utils/ble.js
class BleManager {
  constructor() {
    // ==== 与 ESP32 端 12_Config_Types.h / 05_BLE.ino 完全对应 ====
    this.deviceName = 'WakeLight';
    this.serviceId  = '';
    this.rxCharId   = ''; // write
    this.txCharId   = ''; // notify

    this.deviceId = '';
    this.connected = false;
    this.listeners = {};
    this.recvBuf = '';
    this.flushTimer = null;

    // 设备状态镜像（GET 返回 JSON 后会更新）
    this.status = {
      time: '--:--', mode: 'Default',
      auto: false, brightLevel: 1,
      temp: 0, hum: 0, lightRaw: 0, ble: false
    };
  }

  /* ============ 事件订阅 ============ */
  on(evt, cb) {
    (this.listeners[evt] = this.listeners[evt] || []).push(cb);
  }
  off(evt, cb) {
    if (!this.listeners[evt]) return;
    this.listeners[evt] = this.listeners[evt].filter(c => c !== cb);
  }
  emit(evt, data) {
    (this.listeners[evt] || []).forEach(cb => {
      try { cb(data); } catch (e) { console.error(e); }
    });
  }

  /* ============ 适配器 / 扫描 ============ */
  openAdapter() {
    return new Promise((resolve, reject) => {
      wx.openBluetoothAdapter({ success: resolve, fail: reject });
    });
  }

  startScan(onFound) {
    wx.onBluetoothDeviceFound(res => {
      res.devices.forEach(d => {
        const name = d.name || d.localName || '';
        if (name === this.deviceName) onFound && onFound(d);
      });
    });
    return new Promise((resolve, reject) => {
      wx.startBluetoothDevicesDiscovery({
        allowDuplicatesKey: false,
        success: resolve,
        fail: reject
      });
    });
  }

  stopScan() {
    return new Promise(r => wx.stopBluetoothDevicesDiscovery({ complete: r }));
  }

  /* ============ 连接流程 ============ */
  async connect(deviceId) {
    this.deviceId = deviceId;

    wx.onBLEConnectionStateChange(res => {
      this.connected = res.connected;
      if (!res.connected) this.emit('disconnected');
    });

    // 1) 建立连接
    await new Promise((res, rej) =>
      wx.createBLEConnection({ deviceId, timeout: 10000, success: res, fail: rej })
    );
    await this._sleep(500);

    // 2) 协商 MTU（可选，部分安卓有效）
    wx.setBLEMTU({ deviceId, mtu: 200, complete: () => {} });

    // 3) 发现服务
    await new Promise((res, rej) =>
      wx.getBLEDeviceServices({ deviceId, success: res, fail: rej })
    );

    // 4) 发现特征值
    await new Promise((res, rej) =>
      wx.getBLEDeviceCharacteristics({
        deviceId, serviceId: this.serviceId,
        success: res, fail: rej
      })
    );

    // 5) 启用 TX 通知
    await new Promise((res, rej) =>
      wx.notifyBLECharacteristicValueChange({
        deviceId, serviceId: this.serviceId,
        characteristicId: this.txCharId,
        state: true, success: res, fail: rej
      })
    );

    // 6) 监听通知数据
    wx.onBLECharacteristicValueChange(res => {
      this._handleReceive(this._ab2str(res.value));
    });

    this.connected = true;
    this.emit('connected');
  }

  disconnect() {
    return new Promise(r => {
      wx.closeBLEConnection({
        deviceId: this.deviceId,
        complete: () => { this.connected = false; r(); }
      });
    });
  }

  /* ============ 发送 ============ */
  async sendCommand(cmd) {
    if (!this.connected) throw new Error('未连接设备');
    const text = cmd + '\n';
    const PKT = 20;                       // 默认 MTU 安全分包
    for (let i = 0; i < text.length; i += PKT) {
      const chunk = text.substring(i, i + PKT);
      await new Promise((res, rej) =>
        wx.writeBLECharacteristicValue({
          deviceId: this.deviceId,
          serviceId: this.serviceId,
          characteristicId: this.rxCharId,
          value: this._str2ab(chunk),
          success: res, fail: rej
        })
      );
      if (text.length > PKT) await this._sleep(30);
    }
  }

  async sendCommands(cmds) {
    for (const c of cmds) {
      await this.sendCommand(c);
      await this._sleep(60);
    }
  }

  /* ============ 高层 API ============ */
  async syncTime() {
    const d = new Date();
    const p = n => (n < 10 ? '0' + n : '' + n);
    // 格式：TIME=YYYYMMDD,HHMMSS
    const date = `${d.getFullYear()}${p(d.getMonth() + 1)}${p(d.getDate())}`;
    const time = `${p(d.getHours())}${p(d.getMinutes())}${p(d.getSeconds())}`;
    const cmd = `TIME=${date},${time}`;
    console.log('[BLE TX] 同步时间:', cmd);
    return this.sendCommand(cmd);
  }
  
  setAuto(on)        { return this.sendCommand(`AUTO=${on ? 1 : 0}`); }
  setBrightLevel(lv) { return this.sendCommand(`BRIGHT=${lv}`); }
  getStatus()        { return this.sendCommand('GET'); }
  setSleepPlan(t1, t2, t3) {
    const c = s => s.replace(':', '');
    const cmd = `SLEEP=${c(t1)},${c(t2)},${c(t3)}`;
    console.log('[BLE TX]', cmd);
    return this.sendCommand(cmd);
  }
  setWakePlan(t1, t2, t3) {
    const c = s => s.replace(':', '');
    const cmd = `WAKE=${c(t1)},${c(t2)},${c(t3)}`;
    console.log('[BLE TX]', cmd);
    return this.sendCommand(cmd);
  }
  setMode(idx, r, g, b, br) {
    return this.sendCommand(`MODE${idx}=${r},${g},${b},${br}`);
  }

  /* ============ 接收处理（拼包 + 解析 JSON） ============ */
  _handleReceive(str) {
    this.recvBuf += str;
    console.log('[BLE RX raw]', str);
  
    // 优先按换行符（设备端发完会带 \n）切包
    while (this.recvBuf.indexOf('\n') >= 0) {
      const idx = this.recvBuf.indexOf('\n');
      const line = this.recvBuf.substring(0, idx).trim();
      this.recvBuf = this.recvBuf.substring(idx + 1);
      if (line) this._processLine(line);
    }
  
    // 兜底：没有换行的情况，250ms 后强制处理
    if (this.flushTimer) clearTimeout(this.flushTimer);
    this.flushTimer = setTimeout(() => {
      if (this.recvBuf.length) {
        this._processLine(this.recvBuf.trim());
        this.recvBuf = '';
      }
      this.flushTimer = null;
    }, 250);
  }
  _processLine(line) {
    console.log('[BLE RX line]', line);
    const s = line.indexOf('{'), e = line.lastIndexOf('}');
    if (s >= 0 && e > s) {
      try {
        const obj = JSON.parse(line.substring(s, e + 1));
        this.status = Object.assign({}, this.status, obj);
        this.emit('status', this.status);
        console.log('[BLE Status]', this.status);
      } catch (err) {
        console.warn('[BLE] JSON 解析失败:', line, err);
      }
    }
    this.emit('data', line);
  }

  /* ============ 工具 ============ */
  _ab2str(buf) {
    const a = new Uint8Array(buf); let s = '';
    for (let i = 0; i < a.length; i++) s += String.fromCharCode(a[i]);
    return s;
  }
  _str2ab(s) {
    const buf = new ArrayBuffer(s.length);
    const v = new Uint8Array(buf);
    for (let i = 0; i < s.length; i++) v[i] = s.charCodeAt(i);
    return buf;
  }
  _sleep(ms) { return new Promise(r => setTimeout(r, ms)); }
}

module.exports = new BleManager();   // 单例
