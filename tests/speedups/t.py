import ctypes, random, struct
from unicorn import *
from unicorn.x86_const import *
code=bytes.fromhex("33c0668bb81421ae006683ff017c066683ff147e0583c164eb63668bb04420ae006683fe017c566683fe147f508a44241b84c0751f0fbf151421ae000fbfc52bc29933c22bc28d04808d04808d04808d0cc1eb293a4424360fbfd70fbfc67d112bc29933c22bc28d048003c88d0c81eb0c2bc29933c22bc2c1e00403c88a44241bfec08844241b0fbec08d04406683b84420ae00810f8567ffffff3b4c2440")
lib=ctypes.CDLL("./fl.so")
SZ=0x1000000
buf=(ctypes.c_uint8*SZ)()
lib.setmem(ctypes.cast(buf,ctypes.c_void_p))
CODE=0x537777; END=0x537812
def mk(rng):
    n=rng.randint(1,60)
    ents=[]
    for i in range(n):
        ents.append((rng.choice([rng.randint(-3,25),rng.randint(1,20),0,1,20,21,-1,0x7fff,-0x8000]), rng.randint(0,255)))
    A=b"".join(struct.pack("<hB",v,b) for v,b in ents)+struct.pack("<hB",-127,0)  # 0xff81
    B=b"".join(struct.pack("<hB",rng.choice([rng.randint(-3,25),rng.randint(1,20),0,1,20,21]),rng.randint(0,255)) for _ in range(n+1))
    return A,B
bad=0
for it in range(20000):
    rng=random.Random(it)
    A,B=mk(rng)
    esp=0x800000
    cnt=rng.choice([0,0,1,rng.randint(0,5)]); sides=rng.randint(-5,10)&0xff
    regs=dict(ecx=rng.getrandbits(32),edx=rng.getrandbits(32),esi=rng.getrandbits(32),edi=rng.getrandbits(32),ebp=rng.getrandbits(32),ebx=rng.getrandbits(32))
    if rng.random()<.5: regs['ebp']=(regs['ebp']&0xffff0000)|(rng.randint(1,20)&0xffff)
    # unicorn
    mu=Uc(UC_ARCH_X86,UC_MODE_32)
    mu.mem_map(0x500000,0x100000); mu.mem_write(CODE,code)
    mu.mem_map(0xae0000,0x10000); mu.mem_write(0xae2044,A); mu.mem_write(0xae2114,B)
    mu.mem_map(0x7f0000,0x20000)
    mu.mem_write(esp+0x1b,bytes([cnt])); mu.mem_write(esp+0x36,bytes([sides]))
    for k,v in regs.items(): mu.reg_write({'ecx':UC_X86_REG_ECX,'edx':UC_X86_REG_EDX,'esi':UC_X86_REG_ESI,'edi':UC_X86_REG_EDI,'ebp':UC_X86_REG_EBP,'ebx':UC_X86_REG_EBX}[k],v)
    mu.reg_write(UC_X86_REG_ESP,esp); mu.reg_write(UC_X86_REG_EAX,rng.getrandbits(32))
    # the entry jump: the loop starts at CODE (xor eax,eax)
    try:
        mu.emu_start(CODE,END,count=200000)
    except UcError as e:
        print("emu err",e,it); bad+=1; continue
    exp=[mu.reg_read(r) for r in (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI)]+[mu.mem_read(esp+0x1b,1)[0]]
    # native
    ctypes.memset(buf,0,SZ)
    ctypes.memmove(ctypes.addressof(buf)+0xae2044,A,len(A)); ctypes.memmove(ctypes.addressof(buf)+0xae2114,B,len(B))
    buf[esp+0x1b]=cnt; buf[esp+0x36]=sides
    r=(ctypes.c_uint32*5)(rng.getrandbits(32),regs['ecx'],regs['edx'],regs['esi'],regs['edi'])
    lib.run(esp,regs['ebp'],r)
    got=list(r)+[buf[esp+0x1b]]
    # eax is rewritten by the loop, so compare all
    if got!=exp:
        bad+=1
        if bad<5: print(it,[hex(x) for x in exp],[hex(x) for x in got])
print("mismatches",bad)
