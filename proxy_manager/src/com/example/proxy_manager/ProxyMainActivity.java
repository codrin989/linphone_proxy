package com.example.proxy_manager;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.io.IOException;
import java.net.UnknownHostException;

import org.apache.http.impl.conn.tsccm.WaitingThread;

import android.os.Bundle;
import android.app.Activity;
import android.content.Intent;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class ProxyMainActivity extends Activity {

	EditText textOut;
	TextView textIn;
	String output;
	Intent intentBarcodeScanner;
	int done;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_proxy_main);
		
		intentBarcodeScanner = new Intent("com.google.zxing.client.android.SCAN");
		intentBarcodeScanner.putExtra("SCAN_MODE", "QR_CODE_MODE");
		textOut = (EditText)findViewById(R.id.textout);
	    Button buttonSend = (Button)findViewById(R.id.send);
	    Button buttonScanQR = (Button)findViewById(R.id.QR);
	    textIn = (TextView)findViewById(R.id.textin);
	    buttonSend.setOnClickListener(buttonSendOnClickListener);
	    buttonScanQR.setOnClickListener(buttonGetQRCodeOmClickListener);
	}

	Button.OnClickListener buttonSendOnClickListener
	 = new Button.OnClickListener(){

	@Override
	public void onClick(View arg0) {
	 // TODO Auto-generated method stub
	done = 0;
	  new Thread (new Runnable(){
		  public void run (){
			  System.out.println("in thread");
			  DatagramSocket socket = null;
			 DatagramPacket in = null, out = null;
			 byte[] inData = new byte[1522];
			 byte[] outData = new byte[1522];

			 try {
				  socket = new DatagramSocket(52123);
				  in = new DatagramPacket(inData, inData.length);
				  //dataInputStream = new DataInputStream(socket.getInputStream());
				  //dataOutputStream.writeUTF(textOut.getText().toString());
				  outData = textIn.getText().toString().getBytes();
				  Log.d("proxy debugging", textIn.getText().toString());
				  out = new DatagramPacket(outData, outData.length, InetAddress.getByName("127.0.0.1"), 52124);
				  socket.send(out);
				  Log.d("proxy debugging", "Packet sent");
				  socket.receive(in);
				  output = new String(in.getData()).substring(0, in.getLength());
				  Log.d("proxy debugging", "Packet received " + output);
				  done = 1;
				  //textIn.setText(dataInputStream.readUTF());
				  
			 } catch (UnknownHostException e) {
				  // TODO Auto-generated catch block
				  e.printStackTrace();
			 } catch (IOException e) {
				  // TODO Auto-generated catch block
				  e.printStackTrace();
			 }
			 finally{
				  if (socket != null){
					  try {
						  socket.close();
					   } catch (Exception e) {
						    // TODO Auto-generated catch block
						    e.printStackTrace();
					   }
				  }
			 }
		  }
	  }).start();
	  
	  for (int i = 0; i < 10000000 && done == 0; i++);
	  if (done == 1)
		  textOut.setText(output);
	  else
		  textOut.setText("TIMEOUT !!!");
	  
	
	}};
	
	/* QR Scan button listener */
	Button.OnClickListener buttonGetQRCodeOmClickListener = new Button.OnClickListener() {
		
		@Override
		public void onClick(View arg0) {
			startActivityForResult(intentBarcodeScanner, 0);
		}
	};
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.proxy_main, menu);
		return true;
	}

	/* get from Barcode scanner application */
	 public void onActivityResult(int requestCode, int resultCode, Intent intent) {
	    if (requestCode == 0) {
	        if (resultCode == RESULT_OK) {
	            // contents contains whatever was encoded
	            String contents = intent.getStringExtra("SCAN_RESULT");

	            // Format contains the type of code i.e. UPC, EAN, QRCode etc...
	            String format = intent.getStringExtra("SCAN_RESULT_FORMAT");
	            
	            textIn.setText(contents);

	        }
	    }
	}
}
