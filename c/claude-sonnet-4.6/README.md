# SHA1 Challenge — C Implementation (claude-sonnet-4.6)

โปรแกรมคำนวณ SHA1 แบบวนซ้ำ โดยนำผลลัพธ์ (hex string 40 ตัวอักษร) ของรอบก่อนมาเป็น input ของรอบถัดไป เริ่มต้นจากข้อความ `"clubhouse"`

---

## หลักการที่ใช้ให้คำนวณเร็ว

### 1. Precomputed SHA1 Padding

SHA1 กำหนดให้ message ต้องถูก pad ให้ครบ 512 bits (64 bytes) ก่อนประมวลผล  
เนื่องจากทุก round ใช้ input ขนาดคงที่ **40 bytes** เสมอ โครงสร้าง padding จึงไม่เคยเปลี่ยน:

```
[msg 40B] [0x80] [22 × 0x00] [0x00 0x00 0x00 0x00 0x00 0x00 0x01 0x40]
                                └─────── length = 320 bits ──────────┘
```

ดังนั้นจึง **precompute ส่วน padding ไว้ใน static buffer ล่วงหน้าครั้งเดียว** และทุก round
แค่ `memcpy` เฉพาะ 40 bytes แรกเข้าไปในบล็อก

### 2. ข้ามขั้นตอน SHA1_Init() / SHA1_Update() / SHA1_Final()

OpenSSL API มาตรฐานต้องผ่าน 3 ฟังก์ชัน (Init → Update → Final) ซึ่งมี overhead จากการตรวจสอบ state, copy buffer, และการ finalize  
Implementation นี้ **เรียกตรงไปที่ `SHA1_Transform()`** ซึ่งเป็น low-level single-block compression function:

```c
ctx.h0 = 0x67452301u;   // ← set SHA1 IV โดยตรง ไม่เรียก SHA1_Init()
ctx.h1 = 0xEFCDAB89u;
ctx.h2 = 0x98BADCFEu;
ctx.h3 = 0x10325476u;
ctx.h4 = 0xC3D2E1F0u;

SHA1_Transform(&ctx, block);  // ← compress 1 block โดยตรง
```

ทำให้ loop body แต่ละรอบแทบไม่มี overhead นอกจาก SHA1 computation จริงๆ

### 3. Lookup Table สำหรับ Hex Encoding

แทนที่จะใช้ `sprintf()` หรือแปลง byte → hex ด้วย arithmetic ทุกครั้ง  
โปรแกรมสร้าง **lookup table `HEX16[256]` เก็บค่า `uint16_t`** (2 hex chars ต่อ 1 byte) ไว้ล่วงหน้า:

```c
HEX16[0xFF] = 'f' | ('f' << 8)   // → "ff" ใน 2 bytes
```

การแปลง SHA1 state (160 bits = 20 bytes → 40 hex chars) ทำด้วย **20 ครั้ง lookup** เท่านั้น ไม่มี branch หรือ division

### 4. Cache-Line Aligned Buffer

```c
static uint8_t block[64] __attribute__((aligned(64)));
```

บังคับให้ `block` อยู่ที่ขอบ cache line 64 bytes พอดี ทำให้ `SHA1_Transform()` อ่านข้อมูลได้ใน **1 cache-line fetch** ไม่มี cache line split

### 5. Compiler Flags

```
-O3 -march=native -funroll-loops -fomit-frame-pointer -flto
```

| Flag | ผล |
|---|---|
| `-O3` | aggressive optimization รวม vectorization |
| `-march=native` | ใช้ instruction set ของ CPU จริง (AVX2, SHA-NI ฯลฯ) |
| `-funroll-loops` | unroll loop ลด branch overhead |
| `-fomit-frame-pointer` | คืน register ให้ compiler จัดสรร |
| `-flto` | Link-Time Optimization รวม inlining ข้าม translation unit |

---

## การคอมไพล์

```bash
gcc -O3 -march=native -funroll-loops -fomit-frame-pointer -flto \
    -o sha1 sha1.c -lcrypto \
    -DOPENSSL_API_COMPAT=0x10100000L
```

> ต้องติดตั้ง OpenSSL development library ก่อน:
> ```bash
> sudo apt install libssl-dev   # Debian/Ubuntu
> ```

---

## การรัน

```bash
./sha1 <จำนวนรอบ>
```

**ตัวอย่าง:**

```bash
# รอบที่ 1 (SHA1 ของ "clubhouse" ครั้งแรก)
$ ./sha1 1
1313994e55ed4bbe79d2b04e4529ee2f4ac288f5

# รอบที่ 3
$ ./sha1 3
5c7c81615ad8050363c911c7b5e5ed8661be80d6

# รอบที่ 5,555,555,555 (เป้าหมายหลัก)
$ time taskset -c 0 ./sha1 5555555555
```

> `taskset -c 0` ตรึง process ไว้ที่ core เดียว เพื่อให้การวัดเวลาสม่ำเสมอ

---

## ข้อสังเกต

- โปรแกรมทำงาน **single-threaded** เนื่องจากแต่ละรอบต้องรอผลจากรอบก่อน (sequential dependency)
- `SHA1_Transform()` ถูก deprecated ใน OpenSSL 3.0 จึงใช้ `-DOPENSSL_API_COMPAT=0x10100000L` เพื่อระงับ warning
