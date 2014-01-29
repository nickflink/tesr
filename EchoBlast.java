import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.Date;
import java.util.concurrent.TimeUnit;


public class EchoBlast extends Thread {
    private static final int PORT_NUM = 1989;
    private static final int NUM_PACKETS = 50;
    private static final int ECHO_BLAST_TIME_LIMIT = 500;

    private DatagramSocket dsock = null;
    private boolean producerFinished = false;
    private boolean consumerFinished = false;
    private Thread consumerThread = null;
    private Thread producerThread = null;
    private int resultResponses = 0;
    private long [] resultTimes = new long[NUM_PACKETS];
    class Producer implements Runnable {
        private static final String INET_ADDR = "127.0.0.1";
        private DatagramSocket dsock = null;
        private EchoBlast callback = null;
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
                    //System.out.println("Sent "+i+"/"+NUM_PACKETS);
                    System.out.print("s");
                }
            } catch (java.net.UnknownHostException e) {
                System.out.println("caught java.net.UnknownHostException");
            } catch (java.net.SocketException e) {
                System.out.println("caught java.net.SocketException");
            } catch (java.io.IOException e) {
                System.out.println("caught java.io.IOException");
            }
            //System.out.println("< sendUdp");
            callback.onProducerFinished();
        }
    }
    class Consumer implements Runnable {
        private static final String INET_ADDR = "127.0.0.1";
        private DatagramSocket dsock = null;
        private EchoBlast callback = null;
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
                while(true && count < NUM_PACKETS) {
                    //System.out.println("> creating buffer with length "+bufSize);
                    byte[] buf = new byte[bufSize];
                    DatagramPacket dpack = new DatagramPacket(buf, buf.length);
                    //System.out.println("> dsock.receive");
                    dsock.receive(dpack); // receive the packet
                    ++count;
                    //System.out.println("> dsock.close");
                    String message = new String(dpack.getData( ));
                    Date receiveTime = new Date( ); // note the time of receiving the message
                    long cTime = receiveTime.getTime();
                    //System.out.println("> about to Long.parseLong("+message+")");
                    long sTime = Long.parseLong(message.trim());
                    System.out.print("r");
                    //System.out.println("cTime "+cTime);
                    //System.out.println("sTime "+sTime);
                    //System.out.println("eTime "+(cTime-sTime));
                    //System.out.println("recv "+count+" packet(s)");
                    callback.onResultReceived(cTime-sTime);
                }
            } catch (java.net.UnknownHostException e) {
                System.out.println("caught java.net.UnknownHostException");
            } catch (java.net.SocketException e) {
                System.out.println("caught java.net.SocketException");
            } catch (java.io.IOException e) {
                System.out.println("caught java.io.IOException");
            } catch (java.lang.NumberFormatException e) {
                System.out.println("caught java.lang.NumberFormatException");
            //} catch (java.lang.InterruptedException e) {
            //    System.out.println("caught java.lang.InterruptedException");
            }
            //System.out.println("< recvUdp");
            callback.onConsumerFinished();
        }
    }

    public EchoBlast() {
        for(int i = 0; i < NUM_PACKETS; i++) {
            this.resultTimes[i] = -1;
        }
    }

    private void onResultReceived(long time) {
        this.resultTimes[this.resultResponses] = time;
        ++resultResponses;
    }

    private void onProducerFinished() {
        //System.out.println("> onProducerFinished");
        this.producerFinished = true;
    }

    private void onConsumerFinished() {
        //System.out.println("> onConsumerFinished");
        this.consumerFinished = true;
    }

    private boolean finished() {
        //System.out.println("> finished(producerFinished && consumerFinished)="+producerFinished+" && "+consumerFinished+"\n");
        return producerFinished && consumerFinished;
    }

    private void collate() {
        System.out.println("> collate()");
        double percentageResponse = (double)this.resultResponses/(double)NUM_PACKETS;
        double responseSum = 0.0;
        for(long result : this.resultTimes) {
            if(result < 0.0) {
                responseSum += (double)ECHO_BLAST_TIME_LIMIT;
            } else {
                responseSum += (double)result;
            }
        }
        System.out.println("RESPONSE PERCENT: ("+this.resultResponses+"/"+NUM_PACKETS+")="+(percentageResponse*100)+"%");
        System.out.println("RESPONSE TIME AVG: "+(responseSum/(double)NUM_PACKETS)+" millis");
    }

    @Override
    public void interrupt(){
        System.out.println("> interrupt()");
        try {
            super.interrupt();
            this.dsock.close();
            System.out.println("> producerThread.join()");
            if(this.producerThread != null) {
                this.producerThread.join();
            }
            System.out.println("> consumererThread.join()");
            if(this.consumerThread != null) {
                this.consumerThread.join();
            }
        } catch(java.lang.InterruptedException e) {
            System.out.println("caught java.lang.InterruptedException");
        }
        System.out.println("< interrupt()");
    }

    public void run() {
        System.out.println("> run()");
        Date startTime = new Date();
        Date currentTime = startTime;
        long elapsed = 0;
        try {
            this.dsock = new DatagramSocket();
            Consumer consumerTask = new Consumer(this.dsock, this);
            Producer producerTask = new Producer(this.dsock, this);
            this.consumerThread = new Thread(consumerTask);
            this.producerThread = new Thread(producerTask);
            this.consumerThread.start();
            this.producerThread.start();
            
            //busy wait
            while(!finished()) {
                currentTime = new Date();
                elapsed = currentTime.getTime() - startTime.getTime();
                if(elapsed > ECHO_BLAST_TIME_LIMIT) {
                    interrupt();
                }
            }
        } catch (java.net.SocketException e) {
            System.out.println("caught java.net.SocketException");
        }
        collate();
        System.out.println("ELAPSED TIME: "+elapsed+" millis");
        System.out.println("< run()");
    }

    public static void main( String args[] ) throws Exception {
        System.out.println("> main");
        EchoBlast eb = new EchoBlast();
        eb.start();
        System.out.println("< main");
    }
}

