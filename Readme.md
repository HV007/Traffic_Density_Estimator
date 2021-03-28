Compilation: `make` or `make all` <br/>
Parameters: Update method parameters in `config.json` <br/>
Executing: `./program <video_file> <empty_image>` <br/>
<br/>
The empty image will be shown. Select the four corners of the road. The video will be processed and the output will be printed in csv format (saved to out_file if provided). <br/>
<br/>
Install dependenices: `pip3 install pandas plotly` <br/>
Displaying graph: `python3 plot_graph.py <output_file>` </br>
Install mpstat: `sudo apt install sysstat` <br/>


Remove executable: `make clean` <br/>