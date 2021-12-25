import os.path
import os
from urllib.request import urlopen
from urllib.request import urlretrieve

# downloading deep10m dataset
yadisk_key = 'https://yadi.sk/d/11eDCm7Dsn9GA'
response = urlopen('https://cloud-api.yandex.net/v1/disk/public/resources/download?public_key=' \
    + yadisk_key + '&path=/deep10M.fvecs')
response_body = response.read().decode("utf-8")
dataset_url = response_body.split(',')[0][9:-1]
filename = os.path.join('downloads', 'deep-image.fvecs')
if not os.path.isfile(filename):
    print('Downloading: ' + filename)
    try:
        urlretrieve(dataset_url, filename)
    except Exception as inst:
        print(inst)
        print('  Encountered unknown error. Continuing.')
else:
    print('Already downloaded: ' + filename)