# Porcelix
That's my kernel, ha...

版本记录：
2016-04-15：修改myLoader部分
修改调度机制，目前通过定时器完成时间片轮转，强制调度消耗完时间片的任务。增加sem，将命令行任务改成sem触发的。
