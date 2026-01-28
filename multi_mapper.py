import csv
import subprocess
import shutil
import os

# Path constants
CSV_FILE = "maps.csv"
CONF_FILE = "mapgen.conf"
MAPGEN_EXEC = "./sdl_3"
OUTPUT_FILE = "NowaMapa8.tiff"

# Template of mapgen.conf with placeholders in the same order as CSV columns
CONF_TEMPLATE = """#TERRAIN:
octaves={octaves}
tex_w={tex_w}
tex_h={tex_h}
maxaltitude={maxaltitude}
seed={seed}
noise_scale={noise_scale}
#RIVERS:
river_count={river_count}
#CLIMATE:
wind_clamp={wind_clamp}
wind_scale_downward={wind_scale_downward}
river_evaporation={river_evaporation}
sub_scale={sub_scale}
lat_top={lat_top}
lat_bottom={lat_bottom}
"""

def main():
    with open(CSV_FILE, newline="") as f:
        reader = csv.reader(f)
        
        for row in reader:
            # Last column is map name
            *params, mapname = row
            
            # Convert to dictionary matching template keys
            keys = [
                "octaves", "tex_w", "tex_h", "maxaltitude", "seed", "noise_scale",
                "river_count", "wind_clamp", "wind_scale_downward",
                "river_evaporation", "sub_scale", "lat_top", "lat_bottom"
            ]
            
            values = dict(zip(keys, params))

            # Write mapgen.conf
            with open(CONF_FILE, "w") as conf:
                conf.write(CONF_TEMPLATE.format(**values))

            print(f"Running mapgen for {mapname}...")
            subprocess.run([MAPGEN_EXEC], check=True)

            # Rename output file
            mapname_lat = "m_" + mapname + "_"+str(values["lat_top"])+str(values["lat_bottom"])
            new_name = f"{mapname_lat}.tiff"
            if os.path.exists(OUTPUT_FILE):
                shutil.move(OUTPUT_FILE, new_name)
                print(f"Saved {new_name}")
            else:
                print(f"Warning: {OUTPUT_FILE} not found after running mapgen")

if __name__ == "__main__":
    main()
