import json



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

set_param(5,1,1,50,True,False)
    

