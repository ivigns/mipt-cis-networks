import random
import sys
import string


def main(argv):
    if len(argv) < 4:
        print(f'Usage: {argv[0]} <stations count> <samples count> <data size>')
        return
    
    stations_count = int(argv[1])
    samples_count = int(argv[2])
    data_size = int(argv[3])

    for _ in range(samples_count):
        src_id = random.randint(0, stations_count - 1)
        dst_id = 1024 if random.choices([True, False], weights=[0.1, 0.9])[0] else random.randint(0, stations_count - 1)
        data = ''.join(random.choice(string.ascii_letters) for i in range(data_size))
        print(f'{src_id}\t{dst_id}\t{data}')
    


if __name__ == "__main__":
    main(sys.argv)
