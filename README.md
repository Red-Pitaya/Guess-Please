# Guess-Please
单片机项目

## 一、 总体方案设计

利用现有的STC_B学习板设备，结合数码管滚动显示功能，非易失存储器功能，导航按键功能和蜂鸣器功能，电子音乐功能，485通信功能，将猜测数字大小并给出反馈结果与猜测次数显示在数码管上，实现双人猜测数字小游戏功能。发送方负责给出正确数字（0~99），并存入非易失存储器中，并在收到猜测的数字后发送反馈结果。猜测方负责猜测数字并发给发送方。通过导航键上下左右可以调整数字每一位的大小。

## 二、 实验过程

利用杜邦线连接好两块板子，两人通过485通信进行数据交互，实现实时猜测游戏。游戏最多猜测次数为7次，当已猜过七次未猜中或猜中，会自动结束游戏，分类显示滚动字样“SUCCESS”或“FAIL”。

 - **数码管布局：**
**发送方**：第一二位数码管显示正确数字的大小（0~99），按下K3按键后（进行正确数字存储）第五位数码管显示当前猜测次数，第八位数码管显示大小（0为小，1为大，2为相等）。
**猜测方**：第一二位数码管显示猜测数字的大小（0~99），第八位数码管显示收到的大小（0为小，1为大，2为相等）。
 - **游戏初始化：**
复位后显示滚动字样“GUESS PLEASE”，向里按下导航按键后数码管显示数字，开始游戏。（发送方猜测方一致）
 - **游戏中：**
**发送方**：通过控制导航键上下左右调整正确数字的大小，确认后按下K3进行存储，同时第五，八位数码管都显示0。游戏中可通过按下K2对正确数字进行查询。当收到猜测方猜测的数字后，一两位数码管显示猜测的数字，可通过向里按导航按键进行第八位的调整，0为小，1为大，2为正确。调整完毕后按下K1发送（同时蜂鸣器发声）。每次发送接收一轮回后猜测次数加一（即第五位数码管）
**猜测方**：通过控制导航键上下左右调整猜测数字的大小，确认后按下K1发送（同时蜂鸣器发声）。当收到发送方的大小反馈时（第八位显示）为0或1则可继续调整发送（为0会发出降调的音乐，为1发出升调的音乐）。
 - **游戏结束：**
当发送方发送数字为2（猜测方接收到2）后，数码管滚动显示“SUCCESS”,并于七秒后自动显示“SCORE  <7-猜测次数>”。游戏胜利，结束。
当猜测次数大于七次后，数码管持续滚动显示“FAIL”。游戏失败，结束。

## 三、 设计要求

 - STC_B学习板×2 
 - 杜邦线×2 
 - Keil软件
