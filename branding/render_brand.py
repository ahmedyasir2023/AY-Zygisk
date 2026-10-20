#!/usr/bin/env python3
"""Original programmatic AY Zygisk art. No stock imagery or third-party fonts bundled."""
import math
import random
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter

OUT = Path(__file__).parent
random.seed(21)
FONTS = {
    'heavy': '/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf',
    'regular': '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',
    'mono': '/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf'
}
for k,p in list(FONTS.items()):
    if not Path(p).is_file(): FONTS[k] = '/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf'

def font(size, weight='regular'):
    return ImageFont.truetype(FONTS[weight], size)

def gradient(size):
    w,h = size
    image=Image.new('RGB',size)
    p=image.load()
    for y in range(h):
      for x in range(w):
        t=x/w
        glow=math.exp(-((x/w-.82)/.34)**2-((y/h-.40)/.55)**2)*.13
        hglow=math.exp(-((x/w-.19)/.35)**2-((y/h-.88)/.2)**2)*.05
        p[x,y]=(int(8+19*glow+8*hglow), int(9+13*glow+6*hglow), int(12+7*glow+3*hglow))
    return image

def art_base(size):
    im=gradient(size);d=ImageDraw.Draw(im,'RGBA');w,h=size
    for i in range(40):
        x=int((i+1)*w/40)
        d.line([(x,0),(x-int(h*.08),h)],fill=(190,135,46,10),width=1)
    for i in range(20):
        y=int((i+1)*h/20)
        d.line([(0,y),(w,y)],fill=(190,135,46,10),width=1)
    d.ellipse((int(w*.40),-int(h*.55),int(w*1.25),int(h*.95)),outline=(210,162,72,55),width=2)
    d.ellipse((int(w*.52),-int(h*.31),int(w*1.11),int(h*.78)),outline=(210,162,72,46),width=2)
    for i in range(16):
        x=random.randrange(0,w);y=random.randrange(0,h)
        r=random.choice([1,2]);d.ellipse((x,y,x+r,y+r), fill=(255,216,138,100))
    return im

def bolt(im, cx, cy, scale):
    # Metallic lightning bolt built with a masked gold gradient.
    base=[(35,-105),(-51,19),(13,19),(-12,103),(89,-31),(23,-31)]
    poly=[(int(cx+x*scale),int(cy+y*scale)) for x,y in base]
    mask=Image.new('L',im.size,0)
    md=ImageDraw.Draw(mask);md.polygon(poly,fill=255)
    shine=Image.new('RGBA',im.size,(0,0,0,0));sp=shine.load()
    bx=[v[0] for v in poly];by=[v[1] for v in poly]
    for y in range(max(0,min(by)),min(im.height,max(by)+1)):
      for x in range(max(0,min(bx)),min(im.width,max(bx)+1)):
        mix=.48*(x/im.width)+.52*(y/im.height)
        sp[x,y]=(int(255-63*mix),int(227-104*mix),int(158-103*mix),255)
    blur=Image.new('RGBA',im.size);g=ImageDraw.Draw(blur);g.polygon(poly,fill=(219,153,45,160))
    blur=blur.filter(ImageFilter.GaussianBlur(max(10,int(20*scale))))
    im.alpha_composite(blur)
    im.paste(shine,(0,0),mask)
    d=ImageDraw.Draw(im,'RGBA');d.line(poly+[poly[0]],fill=(255,235,183,142),width=max(2,int(1.7*scale)))

def text(d,pos,txt,sz,col,weight='heavy',spacing=None):
    d.text(pos,txt,font=font(sz,weight),fill=col,spacing=spacing or 4)

def banner(size,name,variant):
    w,h=size;im=art_base(size).convert('RGBA');d=ImageDraw.Draw(im,'RGBA')
    scale=min(w/1600,h/900)
    # Chrome / frame
    m=int(54*scale)
    d.rounded_rectangle((m,m,w-m,h-m),radius=int(20*scale),outline=(193,149,67,80),width=max(2,int(2*scale)))
    d.line((m+45,h-m-35,w-m-45,h-m-35),fill=(187,150,82,90),width=1)
    center_x=int(w*.78);center_y=int(h*.47)
    bolt(im,center_x,center_y,2.1*scale*(1.2 if variant=='wide' else 1))
    d=ImageDraw.Draw(im,'RGBA')
    x=int(w*.095)
    top=int(h*.2)
    text(d,(x,top),'A Y   /   S I G N A T U R E   S E R I E S',int(20*scale),(239,191,105,230),'mono')
    text(d,(x,top+int(105*scale)),'AY ZYGISK',int(110*scale),(249,231,187,255))
    d.rounded_rectangle((x,top+int(257*scale),x+int(355*scale),top+int(311*scale)),radius=17,fill=(181,131,52,45),outline=(225,170,80,125),width=2)
    text(d,(x+int(24*scale),top+int(269*scale)),'BLACK  &  GOLD',int(19*scale),(242,202,123,255),'mono')
    text(d,(x,top+int(368*scale)),'INDEPENDENT CONTROL CENTER',int(26*scale),(218,210,197,240),'regular')
    text(d,(x,top+int(420*scale)),'MAGISK  /  KERNELSU  /  APATCH',int(19*scale),(143,135,122,255),'mono')
    text(d,(x,int(h*.82)),'DEVELOPED BY  ENG. AHMED YASIR',int(21*scale),(223,182,99,255),'mono')
    text(d,(w-int(340*scale),int(h*.82)),'v0.1  |  ALPHA',int(16*scale),(141,125,101,225),'mono')
    output=OUT/name;im.convert('RGB').save(output,optimize=True,quality=94)
    return output

def logo(size=1024):
    im=Image.new('RGBA',(size,size),(7,8,11,255));d=ImageDraw.Draw(im,'RGBA')
    for i in range(60):
        margin=32+i*5
        alpha=max(0,150-i*3)
        if margin<size//2:
            d.rounded_rectangle((margin,margin,size-margin,size-margin),radius=220-i*2,outline=(196,142,48,alpha),width=2)
    d.rounded_rectangle((70,70,size-70,size-70),radius=200,fill=(17,17,19,255),outline=(215,169,79,195),width=8)
    bolt(im,size//2,size//2-15,3.3)
    d=ImageDraw.Draw(im,'RGBA')
    text(d,(size//2-168,830),'AY ZYGISK',60,(238,194,113,255))
    im.save(OUT/'AY-Zygisk-Logo-1024.png')

if __name__=='__main__':
    banner((1600,900),'AY-Zygisk-Banner-1600x900.png','standard')
    banner((1920,720),'AY-Zygisk-Cover-1920x720.png','wide')
    banner((1200,1200),'AY-Zygisk-Social-1200x1200.png','square')
    logo()
    print('Saved',len(list(OUT.glob('*.png'))),'images')
