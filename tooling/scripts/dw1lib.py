import struct
def load(path):
    f=open(path,'rb').read()
    shoff,=struct.unpack('>I',f[0x20:0x24]); se,sn,si=struct.unpack('>HHH',f[0x2e:0x34])
    S=[struct.unpack('>IIIIIIIIII',f[shoff+i*se:shoff+(i+1)*se]) for i in range(sn)]; so=S[si][4]
    nm=lambda o:f[so+o:f.index(b'\0',so+o)].decode()
    d=next(s for s in S if nm(s[0])=='.debug'); d=f[d[4]:d[4]+d[5]]
    dies={}; order=[]; p=0
    while p+4<=len(d):
        ln,=struct.unpack('>I',d[p:p+4]); e=p+max(ln,4)
        if ln<8: p=e; continue
        tag,=struct.unpack('>H',d[p+4:p+6]); q=p+6; A={}
        while q<e:
            at,=struct.unpack('>H',d[q:q+2]); q+=2; fm=at&15
            if fm in (1,2,6): v=struct.unpack('>I',d[q:q+4])[0]; q+=4
            elif fm==5: v=struct.unpack('>H',d[q:q+2])[0]; q+=2
            elif fm==7: v=d[q:q+8]; q+=8
            elif fm==3: n,=struct.unpack('>H',d[q:q+2]); v=d[q+2:q+2+n]; q+=2+n
            elif fm==4: n,=struct.unpack('>I',d[q:q+4]); v=d[q+4:q+4+n]; q+=4+n
            elif fm==8: z=d.index(b'\0',q); v=d[q:z].decode('latin1'); q=z+1
            A[at&0xfff0]=v
        dies[p]=(tag,A,e); order.append(p); p=e
    return dies, order
