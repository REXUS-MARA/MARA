# Mara F´ project

To clone the project and get F' running, do the following
1. (Not necessary, but reccomended) Create a python venv ().
2. Make sure that Python is **not** in version 3.14 (some bug in F' 4.1.0)
3. Install fprime-bootstrap (`pip install fprime-bootstrap`)
4. `fprime-bootstrap clone https://github.com/REXUS-MARA/MARA.git` (alternatively, you can also use git clone and then create the fprime-venv manually, but this is the reccomended way).
5. When running the fprime commands, remember to have your venv activated (`source fprime-venv/bin/activate`)

# Building & deploying
For more information about deployment, go here - https://fprime.jpl.nasa.gov/latest/docs/tutorials/cross-compilation/ .
I'll describe the process for Mac (it's only one of couple ways, this one I just found easiest).

We'll start with gathering requirements. We'll be cross-compiling using a docker image, so you need to have docker installed.
I reccomend colima + docker cli - `brew install docker colima`. After you have that, do the following:
1. In a terminal, run `colima start --vm-type=vz --vz-rosetta -f`. It's important to add --vz-rosetta - the default qemu in my experience is not adequate, and your comlipation will crash (and also it will be slow before it crashes. Rosetta is also slow but not as much). I also run it with `-f` option, to keep it in the foreground. You can skip that and run it in the background, but it will be running a very small vm in the background, slowly eating at your battery after you forget it for a month. 
2. In a new terminal (preferably within the repo rood being your current pwd), run ```docker run --platform=linux/amd64 --net host -e USER=$USER -u "`id -u`:`id -g`" -v "$(pwd):/project" -it ghcr.io/filipjsowa/fprime-arm:latest``` . some important things `--net host` allows you to use your computer network, important for sending the deployment over scp. `-v "$(pwd):/project"` is attaching the current pwd as a volume, that's why it's important where you call it. But you can just change it to the root path. The last thing - I know that the tutorial calls for `nasafprime/fprime-arm:latest` as the image. I've built my own on top of this one, for more info you can go here - https://github.com/nasa/fprime/issues/4837 .
3. ---BELOW INSTRUCTIONS ARE IDENTICAL TO THE ONES IN THE TUTORIAL, IT'S JUST EASIER TO HAVE THEM HERE---
4. `fprime-util generate aarch64-linux` (you may need to run`fprime-util generate -f aarch64-linux` if for some reason that architecture was already present)
5. `fprime-util build aarch64-linux` (this will take some time. Unless you're building on linux, then it will take less time I guess? Or if you're on Linux on ARM, then it should be quick. Unless you're building directly on the Pi (linux + arm), that will be also slow. You know what, it will just be slow).
6. Great, now you have the deployment, and the dictionary (you can search more on the fprime website about what those things are). Time to copy the deployment to the pi, and to run the gds. I'll assume that you have the pi connected over the network you your device , you know it's IP address (need that for the GDS), and that you can ssh into it (need that for scp).
7. `scp build-artifacts/aarch64-linux/<name-of-deployment>/bin/<name-of-deployment> <username>@<device-address>:deployment` - still from the container (for my case it's `scp build-artifacts/aarch64-linux/Mara_MaraRPiIP/bin/Mara_MaraRPiIP fsowa@pi-fsowa.local:MARA`)
8. `fprime-gds -n --dictionary build-artifacts/aarch64-linux/<name-of-deployment>/dict/<App Dictionary>.json --ip-client --ip-address <device-address>` - still from the container (again, for my case it's `fprime-gds -n --dictionary build-artifacts/aarch64-linux/Mara_MaraRPiIP/dict/MaraRPiIPTopologyDictionary.json --ip-client --ip-address $(python3 -c "import socket; print(socket.gethostbyname('pi-fsowa.local'))")`)
9. SSH into your Pi
10. `./MARA -a 0.0.0.0 -p 50000`

# On Python and Homebrew (section only for people starting with programming)
As you can see above, we're usually using python for activating / creating the virtual environments, and brew for installing packages.

Brew (or, in it's full name, Homebrew) is a package manager. It's responsibility is to install packages (a common name for software distributed in a way that lets user install it very easily), uninstall them and update them. Ideally, everything on your system would be installed using a package manager, so I really recommend to install it. Here you have the instructions - https://brew.sh (it's one line to copy and paste into your terminal).

Python is a programming language. One of the cool thing about it is that we have new versions released often, adding improvements - and the same philosophy migrated to to it's packages. It is also the worst thing about python and it's packages - if you'll just install python through brew, or using some bash scripts, you'll very quickly learn that it's close to impossible to have everything in the correct versions. Hence, the need for python virtual environments (for managing versions of packages, and generally not polluting your system), but also a need for python versions managers. I know that conda exists, but I never got to properly use it - I use `uv`, and I'm going to write about it. 

To install `uv` (I assume that you have `brew` already installed) - just do `brew install uv`. 

Now, we can get our venv and fprime-bootstrap as a test project. Assuming you have brew and uv installed, it's enough to do:
1. `uv venv --python 3.12`
2. `source .venv/bin/activate`
3. `uv pip install fprime-bootstrap`
That's it. As you can see - you just choose the python version for the venv. To go out of the venv, just write `deactivate`. `uv` is written in Rust, from my experience is extremely quick.


There are probably typos in above document (especailly in the commands, I typed some of them from memory). Please fix them when spotted.


Some of my notes of things that are lackign from fprime tutorials:
1. A proper setup for docker (I think that what they provide is lacking, I like mine more)
2. man pages don't work? 

**For documentation on F' visit:** https://fprime.jpl.nasa.gov.
