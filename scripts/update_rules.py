import configparser, shutil, glob, os, random, string
import stat, logging, argparse
from git import InvalidGitRepositoryError, repo as GitRepo

def del_rw(action, name: str, exc):
    os.chmod(name, stat.S_IWRITE)
    os.remove(name)

def open_repo(path: str):
    if os.path.exists(repo_path) == False: return None
    try:
        return GitRepo.Repo(path)
    except InvalidGitRepositoryError:
        return None

parser = argparse.ArgumentParser()
parser.add_argument("-c", "--config", default="rules_config.conf")
args = parser.parse_args()

config = configparser.ConfigParser()
config.read(args.config)
logging.basicConfig(format="%(asctime)s %(message)s", level=logging.DEBUG)

for section in config.sections():
    repo = config.get(section, "name", fallback=section)
    url = config.get(section, "url")
    commit = config.get(section, "checkout")
    matches = config.get(section, "match").split("|")
    save_path = config.get(section, "dest", fallback="base/rules/{}".format(repo))
    keep_tree = config.getboolean(section, "keep_tree", fallback=True)

    logging.info("reading files from url {} with commit {} and matches {}, save to {} keep_tree {}".format(url, commit, matches, save_path, keep_tree))

    # ran_str = ''.join(random.sample(string.ascii_letters + string.digits, 16))
    repo_path = os.path.join("./tmp/repo/", repo)
    
    r = open_repo(repo_path)
    if r != None:
        logging.info("repo {} exists, checking out...".format(repo_path))
        r.git.checkout(commit)
    else:
        logging.info("repo {} not exist, cloning...".format(repo_path))
        shutil.rmtree(repo_path, ignore_errors=True)
        os.makedirs(repo_path, exist_ok=True)
        r = GitRepo.Repo.clone_from(url, repo_path)
        r.git.checkout(commit)

    os.makedirs(save_path, exist_ok=True)

    for pattern in matches:
        files = glob.glob("{}/{}".format(repo_path, pattern), recursive=True)
        for file in files:
            if os.path.isdir(file): continue

            (file_rel_path, file_name) = os.path.split(file.removeprefix(repo_path))
            if keep_tree: 
                file_dest_dir = "{}{}".format(save_path, file_rel_path)
                file_dest_path = "{}/{}".format(file_dest_dir, file_name)
                os.makedirs(file_dest_dir, exist_ok=True)
            else:
                file_dest_path = "{}{}".format(save_path, file_name)

            shutil.copyfile(file, file_dest_path)
            logging.info("copied {} to {}".format(file, file_dest_path))

shutil.rmtree("./tmp", onerror=del_rw)
