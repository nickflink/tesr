import java.net.*;
import java.util.*;


public class EchoBlast {
    private static final int PORT_NUM = 1989;
    private static final int NUM_PACKETS = 50;
    private static final int SOCKET_TIMEOUT = 500;

    private static boolean producerFinished = false;
    private static boolean consumerFinished = false;
    class Producer implements Runnable {
        private static final String INET_ADDR = "127.0.0.1";
        private DatagramSocket dsock;
        private EchoBlast callback;
        public Producer(DatagramSocket dsock, EchoBlast callback) {
            this.dsock = dsock;
            this.callback = callback;
        }
        public void run() {
            System.out.println("> sendUdpThread");
            try {
                InetAddress add = InetAddress.getByName(INET_ADDR);
                for(int i = 0; i < NUM_PACKETS; i++) {
                    Date sendTime = new Date( );
                    //ok
                    String sentTimeMsg = ""+sendTime.getTime();
                    byte arr[] = sentTimeMsg.getBytes( );
                    DatagramPacket dpack = new DatagramPacket(arr, arr.length, add, PORT_NUM);
                    dsock.send(dpack);// send the packet
                    System.out.println("Sent "+i+"/"+NUM_PACKETS);
                }
            } catch (java.net.UnknownHostException e) {
                System.out.println("caught java.net.UnknownHostException");
            } catch (java.net.SocketException e) {
                System.out.println("caught java.net.SocketException");
            } catch (java.io.IOException e) {
                System.out.println("caught java.io.IOException");
            }
            System.out.println("< sendUdp");
            callback.onProducerFinished();
        }
    }
    class Consumer implements Runnable {
        private static final String INET_ADDR = "127.0.0.1";
        private DatagramSocket dsock;
        private EchoBlast callback;
        public Consumer(DatagramSocket dsock, EchoBlast callback) {
            this.dsock = dsock;
            this.callback = callback;
        }
        public void run() {
            System.out.println("> recvUdpThread");
            try {
                InetAddress add = InetAddress.getByName(INET_ADDR);
                System.out.println("> making string");
                String maxLongString = Long.toString(Long.MAX_VALUE);
                System.out.println("> getting length");
                int bufSize = maxLongString.length()+1;
                int count = 0;
                while(true) {
                    System.out.println("> creating buffer with length "+bufSize);
                    byte[] buf = new byte[bufSize];
                    DatagramPacket dpack = new DatagramPacket(buf, buf.length);
                    System.out.println("> dsock.receive");
                    dsock.receive(dpack); // receive the packet
                    ++count;
                    System.out.println("> dsock.close");
                    String message = new String(dpack.getData( ));
                    Date receiveTime = new Date( ); // note the time of receiving the message
                    long cTime = receiveTime.getTime();
                    System.out.println("> about to Long.parseLong("+message+")");
                    long sTime = Long.parseLong(message.trim());
                    System.out.println("cTime "+cTime);
                    System.out.println("sTime "+sTime);
                    System.out.println("eTime "+(cTime-sTime));
                    System.out.println("recv "+count+" packet(s)");
                }
            } catch (java.net.UnknownHostException e) {
                System.out.println("caught java.net.UnknownHostException");
            } catch (java.net.SocketException e) {
                System.out.println("caught java.net.SocketException");
            } catch (java.io.IOException e) {
                System.out.println("caught java.io.IOException");
            } catch (java.lang.NumberFormatException e) {
                System.out.println("caught java.lang.NumberFormatException");
            }
            System.out.println("< recvUdp");
            callback.onConsumerFinished();
        }
    }

    private void onProducerFinished() {
        System.out.println("> producerFinished");
        this.producerFinished = true;
    }

    private void onConsumerFinished() {
        System.out.println("> onConsumerFinished");
        this.consumerFinished = true;
    }

    private boolean finished() {
        return producerFinished && consumerFinished;
    }

    private void run() {
        try {
            DatagramSocket dsock = new DatagramSocket();
            dsock.setSoTimeout(SOCKET_TIMEOUT);
            Consumer consumerTask = new Consumer(dsock, this);
            Producer producerTask = new Producer(dsock, this);
            Thread consumerThread = new Thread(consumerTask);
            Thread producerThread = new Thread(producerTask);
            consumerThread.start();
            producerThread.start();
        } catch (java.net.SocketException e) {
            System.out.println("caught java.net.SocketException");
        }
    }

    public static void main( String args[] ) throws Exception {
        EchoBlast eb = new EchoBlast();
        eb.run();
        boolean finished = false;
        while(!finished) {
            Thread.sleep(1);
            if(eb.finished()) {
                finished = true;
            }
        }
    }
}

