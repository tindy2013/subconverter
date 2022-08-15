import sys

try:
    
    txt = sys.argv[1]
    toml = sys.argv[2]
    ret = ""
    for x in open(txt).readlines():
        
        ret+= """[[emoji]]
match = '{0}'
emoji = '{1}'

""".format(x.split(',')[0].strip(),x.split(',')[1].strip())
except: exit("insufficient parameter. <txt path> <toml path>")

with open(toml,'w') as f:
    f.write(ret.strip())
