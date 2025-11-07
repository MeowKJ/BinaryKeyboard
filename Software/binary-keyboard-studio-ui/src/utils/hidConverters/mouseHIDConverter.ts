import type { MouseConfig } from "@/types";

export const getMouseConfigString = (config: MouseConfig): string => {
  const { button, wheel } = config;

  // 按位解析鼠标按键是否按下
  const buttonDescriptions = [
    (button & 0b0001) !== 0 ? "左键" : "",
    (button & 0b0010) !== 0 ? "右键" : "",
    (button & 0b0100) !== 0 ? "中键" : "",
    (button & 0b1000) !== 0 ? "后退" : "",
    (button & 0b10000) !== 0 ? "前进" : "",
  ].filter(Boolean); // 过滤掉空字符串

  // 如果滚轮值不为 0，则添加滚轮信息
  if (wheel !== 0) {
    buttonDescriptions.push(`滚轮: ${wheel}`);
  }

  // 将按键描述数组用 " + " 连接成字符串
  return buttonDescriptions.join(" + ");
};
