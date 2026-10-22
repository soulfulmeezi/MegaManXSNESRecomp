#!/usr/bin/env python3
"""Non-destructive eight-pose 288x288 Civilian Zero run audit (Pillow)."""
import argparse
import json
from pathlib import Path
from PIL import Image, ImageDraw

def iou(a, b):
    x=a.getchannel("A").tobytes(); y=b.getchannel("A").tobytes()
    both=sum(p>0 and q>0 for p,q in zip(x,y))
    either=sum(p>0 or q>0 for p,q in zip(x,y))
    return round(both/either,5) if either else 1

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--frames-dir",type=Path,required=True)
    p.add_argument("--output-dir",type=Path,required=True)
    a=p.parse_args(); a.output_dir.mkdir(parents=True,exist_ok=True)
    frames=[]; errors=[]; warnings=[]; info=[]
    for n in range(8):
        path=a.frames_dir/f"{n:02d}.png"
        if not path.is_file():
            errors.append(f"Missing {path.name}"); continue
        with Image.open(path) as source: frame=source.convert("RGBA")
        if frame.size!=(288,288):errors.append(f"{path.name}: expected 288x288; got {frame.size}")
        bbox=frame.getchannel("A").getbbox()
        if not bbox: errors.append(f"{path.name}: transparent frame")
        info.append({"frame":n,"bbox":bbox})
        frames.append(frame)
    pairs=[]
    if len(frames)==8 and all(f.size==(288,288) for f in frames):
        for n in range(4):
            score=iou(frames[n],frames[n+4])
            pairs.append({"frames":[n,n+4],"mask_iou":score})
            if frames[n].tobytes()==frames[n+4].tobytes():
                errors.append(f"Half-cycle duplicate: {n} and {n+4}")
            elif score>=.95: warnings.append(f"Near duplicate silhouette: {n} and {n+4}")
        sheet=Image.new("RGBA",(1152,640),(32,32,40,255))
        draw=ImageDraw.Draw(sheet)
        for n,frame in enumerate(frames):
            x=(n%4)*288;y=(n//4)*320
            sheet.alpha_composite(frame,(x,y))
            draw.text((x+8,y+294),f"{n} {'NEAR' if n<4 else 'FAR'}",fill="white")
        sheet.save(a.output_dir/"contact_sheet.png")
    result={"status":"FAIL" if errors else "WARN" if warnings else "PRELIMINARY_PASS",
            "errors":errors,"warnings":warnings,"frames":info,"half_cycle":pairs,
            "caveat":"Numerical QC does not prove a completed stride. Compare contact poses visually and in gameplay."}
    (a.output_dir/"report.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(result["status"]); return 1 if errors else 0
if __name__=="__main__": raise SystemExit(main())
