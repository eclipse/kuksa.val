import client
import timer

def stressServer(num, path, file):
    print('stress testing kuksa.val server with setValue')
    Stressclient = client.Client(file)
    Stressclient.start()
    start = time.time_ns()
    for count in range(0, num):
        Stressclient.commThread.setValue('Vehicle.Speed', str(count))
    measure_time.toc()
    print('Elapsed time: %s ns' % measure_time.telapsed_ns)
    one = (measure_time.telapsed_ns / num)
    print('One message is sent all %s ns' % one)
    ratio = 1e9 / one
    print('Pushed %s total messages ' % num, '(%s / s)' % ratio)
    Stressclient.commThread.stop()
