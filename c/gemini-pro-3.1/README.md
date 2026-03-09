# High-Performance SHA1 (SHA-NI) Implementation

โปรแกรมนี้ถูกออกแบบมาเพื่อคำนวณ SHA1 โดยใช้ **Intel SHA Extensions (SHA-NI)** เพื่อประสิทธิภาพสูงสุด โดยเฉพาะสำหรับการคำนวณแบบ Chained Hashing (นำผลลัพธ์ Hex ของรอบก่อนหน้ามาเป็น Input ของรอบถัดไป) เป็นจำนวนหลายล้านรอบ

## Implementation Overview

เพื่อให้รันได้ "เร็วสุดๆ" ทางเราได้ปรับปรุงโค้ดดังนี้:

1.  **Intel SHA Extensions (SHA-NI)**: ใช้คำสั่งชุดคำสั่งระดับ Hardware ผ่าน `<x86intrin.h>` (เช่น `_mm_sha1rnds4_epu32`) ช่วยให้การคำนวณ Block ประมวลผลได้เร็วกว่าการใช้ Soft-SHA ปกติหลายเท่าตัว
2.  **Optimized Message Block**: มีการเตรียม Memory Block ขนาด 64-byte ล่วงหน้าเพียงครั้งเดียวสำหรับการคำนวณรอบถัดๆ ไป (เนื่องจาก Input เป็น Hex String 40 ตัวอักษรเสมอ) จึงไม่ต้องคำนวณ Padding หรือ Length ใหม่ในทุกรอบ
3.  **Fast Hex Conversion**: เปลี่ยนจากการใช้ `sprintf` ที่ช้ามาก มาเป็นการใช้ฟังก์ชัน `state_to_hex` ที่เขียนขึ้นมาเฉพาะทาง เพื่อแปลงผลลัพธ์ Binary เป็น Hex String ลงใน Message Block โดยตรง
4.  **Zero-Overhead Loop**: ตัดลูปที่ไม่จำเป็นภายในฟังก์ชันประมวลผลออก ให้เหลือเพียงการจัดการ 1 Block ตรงๆ สำหรับ Input Chaining
5.  **64-bit Support**: รองรับการระบุจำนวนรอบสูงสุดถึงระดับ `uint64_t` เพื่อรองรับการทำงานหลักพันล้านรอบขึ้นไป

## การคอมไพล์ (Build)

ต้องใช้ CPU ที่รองรับ SHA-NI และเปิด Flag ในคอมไพเลอร์:

```bash
gcc -O3 -msse4.1 -msha sha1.c -o sha1
```

> [!WARNING]
> **Performance Pitfall on Hybrid CPUs (12th Gen+):**
> หลีกเลี่ยงการใช้ `-march=native` เพียงอย่างเดียวบน CPU ที่เป็น Hybrid Architecture (เช่น i7-13620H, i7-14700F) 
> เพราะคอมไพเลอร์อาจพยายามแทรกคำสั่ง AVX เข้ามาปนกับ SSE (SHA-NI) ซึ่งทำให้เกิด **AVX-SSE Transition Penalty** 
> ส่งผลให้ความเร็วตกลงมหาศาล (อาจช้าลงถึง 50 เท่า)
> 
> **ทางแก้:** หากต้องการใช้ `-march=native` ให้พ่วง Flag ปิด AVX ดังนี้:
> `gcc -O3 -march=native -mno-avx -msha sha1.c -o sha1`
```

## การใช้งาน (Usage)

รันโปรแกรมพร้อมระบุจำนวนรอบที่ต้องการ (Default คือ 3 รอบ):

```bash
# ตรวจสอบ 3 รอบ (ผลลัพธ์ต้องได้ 5c7c81615ad8050363c911c7b5e5ed8661be80d6)
./sha1 3

# ทดสอบรัน 10ล้านรอบ (ใช้เวลาประมาณ ~0.5 วินาที)
time ./sha1 10000000

# รัน 5.5 พันล้านรอบ
time taskset -c 0 ./sha1 5555555555
```

## การตรวจสอบความถูกต้อง

ผลลัพธ์ของโปรแกรมนี้จะตรงกับการทำ Pipe Chaining ใน OpenSSL ดังนี้:

```bash
echo -n "clubhouse" | openssl sha1 -r | cut -d" " -f1 | tr -d '\n' | openssl sha1 -r | cut -d" " -f1 | tr -d '\n' | openssl sha1 -r | cut -d" " -f1
```
