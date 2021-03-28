import json
import subprocess
import pandas as pd
import plotly.express as px

def set_param(x,resolve,space_threads,time_threads,space_opt,print_data):
    data={
        "x": x,
        "resolve": resolve,
        "space_threads": space_threads,
        "time_threads": time_threads,
        "space_opt": space_opt,
        "print_data": print_data
    }
    with open("config.json","w") as json_file:
        json.dump(data,json_file)

set_param(5,1,1,5,True,False)

data = []
for i in [1,2,5,10,20]:
    set_param(5, i, 1, 5, True, False)
    process = subprocess.Popen(["./program", "trafficvideo.mp4", "empty.jpg"], stdout = subprocess.PIPE, universal_newlines = True)
    lines = process.stdout.readlines()
    lines = [line.rstrip("\n") for line in lines]
    lines = [float(line) for line in lines]
    data.append(lines)

df = pd.DataFrame(data, columns = ["Queue", "Dynamic", "Runtime"])
print(df)
fig = px.line(df, x = "Runtime", y = ["Queue", "Dynamic"], title = "Graph")
fig.show()
