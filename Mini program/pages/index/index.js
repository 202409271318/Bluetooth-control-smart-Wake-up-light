const ble = require('../../utils/ble.js');

const DEFAULT_AVATAR_URL = '/images/icons/avatar.png';

const DEFAULT_USER = {
  avatarUrl: DEFAULT_AVATAR_URL,
  nickName: '微信用户'
};

Page({
  data: {
    // 用户信息
    userInfo: DEFAULT_USER,
    isLoggedIn: false,
    loginStatusText: '未登录（游客模式）',
    isEditingNickname: false,
    editNicknameValue: '',
    networkTip: '',
    // 蓝牙设备
    scanning: false,
    devices: [],
    connecting: false
  },

  onLoad() {
    this.app = getApp();
    this.loadUserInfo();
    this.detectNetwork();
    
    // 检查是否已连接设备
    if (ble.connected) {
      wx.redirectTo({ url: '/pages/control/control' });
    }
  },

  onShow() {
    this.loadUserInfo();
  },

  /* ============================== 用户信息 / 登录 ============================== */

  loadUserInfo() {
    const raw = this.app.globalData.userInfo || DEFAULT_USER;
    const avatarUrl = raw.avatarUrl && String(raw.avatarUrl).trim()
      ? raw.avatarUrl
      : DEFAULT_AVATAR_URL;
    const localUser = Object.assign({}, raw, { avatarUrl });
    const isLoggedIn = this.isProfileAuthorized();
    this.setData({
      userInfo: localUser,
      isLoggedIn,
      loginStatusText: isLoggedIn ? '已登录（微信用户）' : '未登录（游客模式）'
    });
  },

  isProfileAuthorized() {
    return (
      this.app.globalData.profileAuthorized === true ||
      wx.getStorageSync('profileAuthorized') === true
    );
  },

  handleLogout() {
    wx.removeStorageSync('profileAuthorized');
    this.app.globalData.profileAuthorized = false;
    const guest = Object.assign({}, DEFAULT_USER);
    this.app.setUserInfo(guest);
    this.setData({
      userInfo: guest,
      isLoggedIn: false,
      loginStatusText: '未登录（游客模式）',
      isEditingNickname: false,
      editNicknameValue: ''
    });
    wx.showToast({ title: '已退出登录', icon: 'none' });
  },

  onAvatarAreaTap() {
    if (!this.data.isLoggedIn) {
      wx.showToast({ title: '请先一键登录', icon: 'none' });
    }
  },

  detectNetwork() {
    wx.getNetworkType({
      success: ({ networkType }) => {
        const isOffline = networkType === 'none';
        this.app.globalData.networkConnected = !isOffline;
        this.setData({ networkTip: isOffline ? '无网络连接，显示缓存用户信息' : '' });
      }
    });
  },

  onChooseWechatAvatar(e) {
    if (!this.data.isLoggedIn) {
      wx.showToast({ title: '请先一键登录', icon: 'none' });
      return;
    }
    const url = e.detail && e.detail.avatarUrl;
    if (!url) {
      wx.showToast({ title: '未获取到头像，请重试', icon: 'none' });
      return;
    }
    this.updateAvatar(url);
  },

  updateAvatar(avatarUrl) {
    const newUserInfo = Object.assign({}, this.data.userInfo, { avatarUrl });
    this.app.setUserInfo(newUserInfo);
    const logged = this.isProfileAuthorized();
    this.setData({
      userInfo: newUserInfo,
      isLoggedIn: logged,
      loginStatusText: logged ? '已登录（微信用户）' : '未登录（游客模式）'
    });
    wx.showToast({ title: '头像更新成功', icon: 'success' });
  },

  onAvatarError() {
    if (this.data.userInfo.avatarUrl === DEFAULT_AVATAR_URL) return;
    const newUserInfo = Object.assign({}, this.data.userInfo, { avatarUrl: DEFAULT_AVATAR_URL });
    this.app.setUserInfo(newUserInfo);
    const stillLoggedIn = this.isProfileAuthorized();
    this.setData({
      userInfo: newUserInfo,
      isLoggedIn: stillLoggedIn,
      loginStatusText: stillLoggedIn ? '已登录（微信用户）' : '未登录（游客模式）'
    });
  },

  editNickname() {
    if (!this.data.isLoggedIn) {
      wx.showToast({ title: '请先一键登录', icon: 'none' });
      return;
    }
    this.setData({ isEditingNickname: true, editNicknameValue: this.data.userInfo.nickName });
  },

  onNicknameInput(event) {
    this.setData({ editNicknameValue: event.detail.value });
  },

  saveNickname() {
    if (!this.data.isLoggedIn) {
      wx.showToast({ title: '请先一键登录', icon: 'none' });
      this.setData({ isEditingNickname: false, editNicknameValue: '' });
      return;
    }
    const newNickname = this.data.editNicknameValue.trim();
    if (!newNickname) {
      wx.showToast({ title: '请输入昵称', icon: 'none' });
      return;
    }
    const newUserInfo = Object.assign({}, this.data.userInfo, { nickName: newNickname });
    this.app.setUserInfo(newUserInfo);
    this.setData({ userInfo: newUserInfo, isEditingNickname: false, editNicknameValue: '' });
    wx.showToast({ title: '昵称保存成功', icon: 'success' });
  },

  cancelEditNickname() {
    this.setData({ isEditingNickname: false, editNicknameValue: '' });
  },

  handleAuthorize() {
    wx.getUserProfile({
      desc: '用于展示头像和昵称',
      success: ({ userInfo }) => {
        const cur = this.data.userInfo || {};
        const profileAvatar = userInfo.avatarUrl && String(userInfo.avatarUrl).trim();
        const merged = {
          nickName: userInfo.nickName || cur.nickName,
          avatarUrl: profileAvatar
            ? userInfo.avatarUrl
            : (cur.avatarUrl && String(cur.avatarUrl).trim() ? cur.avatarUrl : DEFAULT_AVATAR_URL)
        };
        wx.setStorageSync('profileAuthorized', true);
        this.app.globalData.profileAuthorized = true;
        this.app.setUserInfo(merged);
        this.setData({
          userInfo: merged,
          isLoggedIn: true,
          loginStatusText: '已登录（微信用户）',
          networkTip: ''
        });
      },
      fail: () => {
        wx.showToast({ title: '未授权，使用默认头像', icon: 'none' });
      }
    });
  },

  /* ============================== 蓝牙扫描/连接（保持原有逻辑） ============================== */

  async startScan() {
    this.setData({ scanning: true, devices: [] });
    try {
      await ble.openAdapter();
      await ble.startScan(device => {
        const list = this.data.devices.slice();
        if (!list.find(d => d.deviceId === device.deviceId)) {
          list.push({
            deviceId: device.deviceId,
            name: device.name || device.localName || '未知',
            rssi: device.RSSI
          });
          this.setData({ devices: list });
        }
      });
      // // ============ 模拟扫描成功 - 开始 ============
      // setTimeout(() => {
      //   this.setData({
      //     devices: [{
      //       deviceId: 'mock_device_id',
      //       name: 'WakeLight',
      //       rssi: -50
      //     }]
      //   });
      // }, 500);
      // // ============ 模拟扫描成功 - 结束 ============
    } catch (e) {
      wx.showToast({ title: '请打开蓝牙', icon: 'none' });
      this.setData({ scanning: false });
    }
  },

  async stopScan() {
    await ble.stopScan();
    // // ============ 模拟停止扫描 - 开始 ============
    // // ============ 模拟停止扫描 - 结束 ============
    this.setData({ scanning: false });
  },

  async tapDevice(e) {
    if (this.data.connecting) return;
    const dev = e.currentTarget.dataset.dev;
    this.setData({ connecting: true });
    wx.showLoading({ title: '连接中...' });

    try {
      await this.stopScan();
      await ble.connect(dev.deviceId);
      // 连接后立刻同步时间 + 拉取状态
      await ble.syncTime();
      await ble.getStatus();
      // // ============ 模拟连接成功 - 开始 ============
      // ble.connected = true;  // 模拟连接状态为已连接
      // // ============ 模拟连接成功 - 结束 ============

      wx.hideLoading();
      wx.showToast({ title: '连接成功', icon: 'success' });
      setTimeout(() => {
        wx.redirectTo({ url: '/pages/control/control' });
      }, 600);
    } catch (e) {
      wx.hideLoading();
      console.error(e);
      wx.showToast({ title: '连接失败', icon: 'none' });
      this.setData({ connecting: false });
    }
  }
});
