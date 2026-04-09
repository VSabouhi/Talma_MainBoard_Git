روی مین بهتر است فقط این کارها بماند:

decode
validity/confidence mapping
bed mapping
synchronization
global health/state
UI output

*************************************************************************************************
Main = Global State Reconstruction + System Coordination
*************************************************************************************************
معماری نرم‌افزاری پیشنهادی داخل Main Board

من برای پروژه تو این ماژول‌ها را پیشنهاد می‌کنم:

zone_analysis.c/.h
movement.c/.h
risk_engine.c/.h
alert_engine.c/.h
recommendation.c/.h
summary_pkt.c/.h یا ادامه‌ی uart_pkt

و یک task یا periodic function مثل:

SummaryTask
یا
update در بازه‌ی 500ms/1s
ورودی Summary Layer

فقط از snapshot پایدار:

bed_value_send
bed_status_send
bed_valid_send
node_state


*************************************************************************************************

پیشنهاد اجرایی خیلی مهم
برای فاز اول summary را این‌طور بساز:
Zone analysis
Movement detection
Risk score ساده
Alert rules
Recommendation
Summary packet

این ترتیب وابستگی‌ها را درست نگه می‌دارد.
