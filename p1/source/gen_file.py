import sys, time, argparse
import socket

def get_args(argv):
    parser = argparse.ArgumentParser(description="File generation tool")
    parser.add_argument('-f', '--filename', required=False, default="test_file.txt")
    parser.add_argument('-b', '--bytes-to-write', required=False, default=100000000, type=int)
    return parser.parse_args()

def generate_big_random_sentences(filename,bytes_to_write):
    import random
    nouns = ("puppy", "car", "rabbit", "girl", "monkey")
    verbs = ("runs", "hits", "jumps", "drives", "leaps")
    adv = ("crazily.", "dutifully.", "foolishly.", "merrily.", "occasionally.")
    adj = ("adorable", "clueless", "dirty", "odd", "stupid")

    all = [nouns, verbs, adv]

    bytes_written = 0
    with open(filename,'w') as f:
        while bytes_written < bytes_to_write:
            line = ' '.join([random.choice(i) for i in all]) + '\n'
            f.writelines(line)
#            print(line)
#            print(len(line))
            bytes_written += len(line)

def main(argv):
    args = get_args(argv)

    generate_big_random_sentences(args.filename, args.bytes_to_write)

if __name__ == "__main__":
    main(sys.argv[1:])
