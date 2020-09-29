from matplotlib.ticker import FuncFormatter
import matplotlib.pyplot as plt
import numpy as np
import sys

funcs = [0.28, 0.3, 2.43, 5.21,  5.54]
code = [0.49, 0.5, 3.85, 8.78, 9.18]
gads = [0.59, 0.64, 4.69, 10.06,  10.81]

zipped = zip(funcs, code, gads)
redis_sys, nginx_sys, sc, redis_app, nginx_app = [list(x) for x in zipped]

index = np.arange(3)
font = {'size'   : 30}
plt.rc('font', **font)


bar_width = 0.125

fig, ax = plt.subplots(figsize=(16, 8))
print redis_sys, nginx_sys, redis_app, nginx_app

bar1 = ax.bar(index - bar_width, redis_sys,
                 bar_width, label=r"Redis$_{2D}$")
bar2 = ax.bar(index, nginx_sys,
                 bar_width, label=r"Nginx$_{2D}$")
bar3 = ax.bar(index + bar_width, sc,
                 bar_width, label=r"SC$_{1D}$")
bar4 = ax.bar(index + 2 * bar_width, redis_app,
                 bar_width, label="Redis$_{1D}$")
bar5 = ax.bar(index + 3 * bar_width, nginx_app,
                 bar_width, label="Nginx$_{1D}$")

x_ticks = ["Functions", "Code size", "ROP gadgets"]
ax.set_xticks(np.arange(len(x_ticks)) + bar_width)
ax.xaxis.set_tick_params(length=0)

ax.set_xticklabels(x_ticks, rotation=0)

ax.legend(ncol=2, loc='upper left').draggable()
ax.set_ylabel("Attack Surface (%)")
plt.tight_layout(2)
plt.show()
