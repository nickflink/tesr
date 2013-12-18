import java.net.*;
import java.util.*;


public class EchoBlast {
    private static final int PORT_NUM = 1989;
    private static final int NUM_PACKETS = 500;
    private static final int SOCKET_TIMEOUT = 500;
    class Producer extends Thread {
        private static final String INET_ADDR = "127.0.0.1";
        private DatagramSocket dsock;
        public Producer(DatagramSocket dsock) {
            this.dsock = dsock;
        }
        public void run() {
            System.out.println("> sendUdpThread");
            try {
                InetAddress add = InetAddress.getByName(INET_ADDR);
                //DatagramSocket dsock = new DatagramSocket( );
                //while(true) {
                for(int i = 0; i < NUM_PACKETS; i++) {
                    Date sendTime = new Date( ); // note the time of sending the message
                    //ok
                    String sentTimeMsg = ""+sendTime.getTime();//"This is client calling";
                    //sentTimeMsg = ""+i;//"This is just the count";
                    //sentTimeMsg = "a"+sendTime.getTime();//"This is client calling";
                    //sentTimeMsg = sendTime.getTime()+"b";//"This is client calling";
                    byte arr[] = sentTimeMsg.getBytes( );
                    DatagramPacket dpack = new DatagramPacket(arr, arr.length, add, PORT_NUM);
                    dsock.send(dpack);// send the packet
                    //dsock.close();
                }
            } catch (java.net.UnknownHostException e) {
                System.out.println("caught java.net.UnknownHostException");
            } catch (java.net.SocketException e) {
                System.out.println("caught java.net.SocketException");
            } catch (java.io.IOException e) {
                System.out.println("caught java.io.IOException");
            }
            System.out.println("< sendUdp");
        }
    }
    class Consumer extends Thread {
        private static final String INET_ADDR = "127.0.0.1";
        private DatagramSocket dsock;
        public Consumer(DatagramSocket dsock) {
            this.dsock = dsock;
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
                    //DatagramSocket dsock = new DatagramSocket( );
                    DatagramPacket dpack = new DatagramPacket(buf, buf.length);
                    System.out.println("> dsock.receive");
                    dsock.receive(dpack); // receive the packet
                    ++count;
                    System.out.println("> dsock.close");
                    //dsock.close();
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
            //} catch (java.net.SocketTimeoutException e) {
            //    System.out.println("caught java.net.SocketTimeoutException");
            }
            System.out.println("< recvUdp");
        }
    }

    private void run() {
        try {
            DatagramSocket dsock = new DatagramSocket( );
            dsock.setSoTimeout(SOCKET_TIMEOUT);
            Consumer consumer = new Consumer(dsock);
            Producer producer = new Producer(dsock);
            consumer.start();
            producer.start();
        } catch (java.net.SocketException e) {
            System.out.println("caught java.net.SocketException");
        }
    }

    public static void main( String args[] ) throws Exception {
        //sendUdp();
        //recvUdp();
        EchoBlast eb = new EchoBlast();
        eb.run();
    }
}// - See more at: http://way2java.com/networking/echo-server-udp/#sthash.Hal1fOQp.dpuf
