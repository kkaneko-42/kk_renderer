import sys, os, glob, subprocess

USAGE = "Usage: python {} <SHADERS_DIR> <COMPILER_PATH>".format(__file__)

def main(argv):
    if len(argv) != 3:
        print(USAGE, file=sys.stderr)
        return 1

    # Gather shader sources
    SHADERS_DIR=argv[1]
    srcs = glob.glob("{}/*.vert".format(SHADERS_DIR), recursive=True)
    srcs += glob.glob("{}/*.frag".format(SHADERS_DIR), recursive=True)

    COMPILER_PATH=argv[2]
    for src in srcs:
        # Build command
        cmd = [COMPILER_PATH, src, "-o", src + ".spv"]

        # Execute command
        print(" ".join(cmd))
        ret = subprocess.run(cmd).returncode
        if ret != 0:
            return 1

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv))
