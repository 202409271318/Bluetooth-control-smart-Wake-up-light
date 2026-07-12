/**
 * @file app.js - 小程序全局应用入口文件
 * 
 * 全局数据：
 *   - profileAuthorized: 是否完成「一键登录」
 *   - userInfo:          用户微信头像和昵称
 *   - networkConnected:  网络连接状态
 *   - deviceInfo:        蓝牙设备连接信息
 */

const DEFAULT_AVATAR_URL = '/images/icons/avatar.png';

const DEFAULT_USER = {
  avatarUrl: DEFAULT_AVATAR_URL,
  nickName: '微信用户'
};

App({
  /** 全局共享数据 */
  globalData: {
    profileAuthorized: false,
    userInfo: Object.assign({}, DEFAULT_USER),
    networkConnected: true,
    deviceInfo: {
      deviceId: '',
      name: '',
      connected: false
    }
  },

  /**
   * 小程序启动入口
   */
  onLaunch() {
    console.log('SmartWakeLight 小程序启动');
    this.loadLocalCache();
  },

  /** 加载本地缓存 */
  loadLocalCache() {
    const userInfo = wx.getStorageSync('userInfo');
    const deviceInfo = wx.getStorageSync('deviceInfo');

    if (userInfo) {
      if (!userInfo.avatarUrl || !String(userInfo.avatarUrl).trim()) {
        userInfo.avatarUrl = DEFAULT_AVATAR_URL;
      }
      this.globalData.userInfo = userInfo;
    }
    if (deviceInfo) {
      this.globalData.deviceInfo = deviceInfo;
    }
    const profileAuthorized = wx.getStorageSync('profileAuthorized');
    this.globalData.profileAuthorized = profileAuthorized === true;
  },

  /** 设置用户信息 */
  setUserInfo(userInfo) {
    this.globalData.userInfo = Object.assign({}, this.globalData.userInfo, userInfo);
    wx.setStorageSync('userInfo', this.globalData.userInfo);
  },

  /** 设置设备信息 */
  setDeviceInfo(deviceInfo) {
    this.globalData.deviceInfo = Object.assign({}, this.globalData.deviceInfo, deviceInfo);
    wx.setStorageSync('deviceInfo', this.globalData.deviceInfo);
  }
});
