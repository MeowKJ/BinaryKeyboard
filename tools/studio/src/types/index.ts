/**
 * 类型导出
 * 
 * 注意：新协议类型在 ./protocol.ts 中定义
 * 本文件保留用于向后兼容
 */

// 重新导出新协议类型
export * from './protocol';

// 按钮形状类型 (UI 相关)
export type ButtonShape = 'square' | 'circle' | 'tall' | 'wide' | 'encoder-cw' | 'encoder-ccw' | 'encoder-press';
