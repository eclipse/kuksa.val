import client
import time
import config

def stressServer(num, path, file):
    print('stress testing kuksa.val server with setValue')
    Stressclient = client.Client(file)
    Stressclient.start()
    start = time.time_ns()
    for count in range(0, num):
        Stressclient.commThread.setValue('Vehicle.Speed', str(count))
    end = time.time_ns()
    elapsed_ns = end - start
    print('Elapsed time: %s ns' % elapsed_ns)
    one = (elapsed_ns / num)
    print('One message is sent all %s ns' % one)
    ratio = 1e9 / one
    print('Pushed %s total messages ' % num, '(%s / s)' % ratio)
    Stressclient.commThread.stop()
