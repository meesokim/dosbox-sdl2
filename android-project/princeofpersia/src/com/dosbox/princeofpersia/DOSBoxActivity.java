package com.dosbox.princeofpersia;

import org.libsdl.app.SDLActivity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.Log;
import android.view.*;
import android.view.Surface;
import android.view.KeyEvent;
import android.view.inputmethod.InputMethodManager;
import android.content.res.AssetManager;
import java.io.*;
import org.libsdl.app.SDLActivity;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Canvas;
import android.widget.Toast;
import java.lang.Math;
import java.util.Map;
import java.util.HashMap;
import android.app.Instrumentation;
import android.util.DisplayMetrics;
import android.support.v4.view.MotionEventCompat;

/* 
 * A sample wrapper class that just calls SDLActivity 
 */ 

public class DOSBoxActivity extends SDLActivity {
    /* Based on volume keys related patch from bug report:
    http://bugzilla.libsdl.org/show_bug.cgi?id=1569     */
    private static final String TAG = "DOSBox";
    // enable to intercept keys before SDL gets them
    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        switch (event.getKeyCode()) {
        // forward volume keys to Android
        case KeyEvent.KEYCODE_VOLUME_UP:
        case KeyEvent.KEYCODE_VOLUME_DOWN:
            return false;
        // Show/Hide on-screen keyboard (but don't change to text input mode)
        case KeyEvent.KEYCODE_BACK:
            if (event.getAction() == KeyEvent.ACTION_UP)
                toggleOnScreenKeyboard();
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    // Fix the initial orientation
    protected void onCreate(Bundle savedInstanceState) {
        /* Use deprecated getOrientation() rather
        than getRotation() to support API<8    */
        int rotation = getWindowManager().getDefaultDisplay().getOrientation();
		if ((rotation == Surface.ROTATION_0) || (rotation == Surface.ROTATION_90))
			setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
		
/*
        int rotation = getWindowManager().getDefaultDisplay().getOrientation();
        if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT) {
            if ((rotation == Surface.ROTATION_0) || (rotation == Surface.ROTATION_90))
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
            else
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT);
        } else { // Landscape
            if ((rotation == Surface.ROTATION_0) || (rotation == Surface.ROTATION_90))
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
            else
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE);
        }
*/
        super.onCreate(savedInstanceState); // Initialize the rest (e.g. SDL)
		copyAssetAll("game");
		copyAssetAll("dosbox.conf");
		idToTouch = new HashMap();
//		MyView m = new MyView(this);
//		mLayout.addView(m);
    }

    public void toggleOnScreenKeyboard() {
        InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.toggleSoftInput(0, 0);
    }

	public void copyAssetAll(String srcPath) {
		AssetManager assetMgr = this.getAssets();
		String assets[] = null;
		try {
			String destPath = getExternalFilesDir(null) + File.separator + srcPath;
			assets = assetMgr.list(srcPath);
			if (assets.length == 0) {
				copyFile(srcPath, destPath);
			} else {
				File dir = new File(destPath);
				if (!dir.exists())
					dir.mkdir();
				for (String element : assets) {
					copyAssetAll(srcPath + File.separator + element);
				}
			}
		} 
		catch (IOException e) {
		   e.printStackTrace();
		}
	}
	public void copyFile(String srcFile, String destFile) {
		AssetManager assetMgr = this.getAssets();
	  
		InputStream is = null;
		OutputStream os = null;
		try {
			is = assetMgr.open(srcFile);
			if (new File(destFile).exists() == false)
			{
				os = new FileOutputStream(destFile);
		  
				byte[] buffer = new byte[1024];
				int read;
				while ((read = is.read(buffer)) != -1) {
					os.write(buffer, 0, read);
				}
				is.close();
				os.flush();
				os.close();
				Log.v(TAG, "copy from Asset:" + destFile);
			}
		} 
		catch (IOException e) {
			e.printStackTrace();
		}
	}
	float[] xx = new float[10];
	float[] yy = new float[10];
	float[] px = new float[10];
	float[] py = new float[10];
	float[] x0 = new float[10];
	float[] y0 = new float[10];
    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
		int action = MotionEventCompat.getActionMasked(event);
		// Get the index of the pointer associated with the action.
		int index = MotionEventCompat.getActionIndex(event);
        // Get id (constant throughout physical touch) corresponding to the action
        final int pointerId = event.getPointerId(index);
		DisplayMetrics metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(metrics);  
		final int width = metrics.widthPixels;
		final int height = metrics.heightPixels;
        switch(action){
			case MotionEvent.ACTION_POINTER_DOWN:
			case MotionEvent.ACTION_DOWN:
			{
				// Treat all pointers as equal
				// i.e. no distinction between primary and secondary
				
				LocalTouch touch = idToTouch.get(pointerId);
				if(null == touch){
					// First one with this id, create it first
					touch = new LocalTouch(width, height);
					idToTouch.put(pointerId, touch);
				}
				touch.onDown(pointerId, event.getX(index), event.getY(index));
				return true;
			}
			case MotionEvent.ACTION_MOVE:
			{
				// If looking for a specific pointer, use
				// int specificPointerIndex = MotionEventCompat.findPointerIndex(event, specificPointerId);
				
				// Multiple pointer motions may be batched in one #onTouchEvent()
				for(int i = 0; i < event.getPointerCount(); i++){
					LocalTouch touch = idToTouch.get(event.getPointerId(i));
					if (touch != null)
						touch.onMove(event.getX(i), event.getY(i));
				}
				return true;
			}
			case MotionEvent.ACTION_UP:
			case MotionEvent.ACTION_CANCEL:
			case MotionEvent.ACTION_POINTER_UP:
			{
				// Like in DOWN, treat all pointers as equal
				idToTouch.get(pointerId).onUp();
				
				// If you prefer, you could remove the Object, i.e. idToTouch.remove(pointerId), here instead
				// eliminating the need for the LocalTouchEvent#valid flag
				
				return true;
			}
        }
        
        return false;
    }		
	private class LocalTouch {
        
        public boolean active;
        
		public float x0, y0;
		public float px, py;
        public int id, key;
		int w, h;
		boolean cursor;
        
		public LocalTouch() {
		}
        public LocalTouch(int width, int height){
            active = false;
			cursor = false;
			this.w = width;
			this.h = height;
        }
        
        public void onDown(int id, float x, float y){
            this.id = id;
            this.x0 = px = x;
            this.y0 = py = y;
            active = true;
			if (x < w/2) 
				cursor = true;
			else {
				SDLActivity.onNativeKeyDown(key = KeyEvent.KEYCODE_SHIFT_LEFT);
			}
        }
        
        public void onMove(float x, float y){
			int len = (int)Math.sqrt((px-x)*(px-x)+(py-y)*(py-y));
			int degree = (int) (Math.atan2(px - x, py - y) * 180 / Math.PI);
			if (len > 30 && cursor) { 
				Log.v("SDL", "dx=" + (int) (px - x) + ",dy=" + (int)(py - y) + ",degree:" + (int) degree + ",len=" + len);
				if (degree > -65 && degree < 65) {
					SDLActivity.onNativeKeyDown(19 /*KeyEvent.UP*/);
					SDLActivity.onNativeKeyUp(20 /*KeyEvent.DOWN*/);
				}	
				if (degree >= 25 && degree < 155) {
					SDLActivity.onNativeKeyDown(21 /*KeyEvent.LEFT*/);
					SDLActivity.onNativeKeyUp(22 /*KeyEvent.RIGHT*/);
				}
				if (degree >= 115 || degree < -115) {
					SDLActivity.onNativeKeyDown(20 /*KeyEvent.DOWN*/);
					SDLActivity.onNativeKeyUp(19 /*KeyEvent.UP*/);
				}
				if (degree < -25 && degree > -155) {
					SDLActivity.onNativeKeyDown(22 /*KeyEvent.RIGHT*/);
					SDLActivity.onNativeKeyUp(21 /*KeyEvent.LEFT*/);
				}
				px = x;
				py = y;
			}			
        }
        
        public void onUp(){
			if (cursor) {
				SDLActivity.onNativeKeyUp(19);
				SDLActivity.onNativeKeyUp(20);
				SDLActivity.onNativeKeyUp(21);
				SDLActivity.onNativeKeyUp(22);
			} else {
				SDLActivity.onNativeKeyUp(key);
			}
			active = false;
			cursor = false;
        }
    }
    
    private Map<Integer, LocalTouch> idToTouch;
}

class MyView extends View {
    Paint paint = new Paint();
    Path path  = new Path();    // 자취를 저장할 객체
	View mView;
	float px, py;
	int width, heigth;
    public MyView(Context context) {
        super(context);
        paint.setStyle(Paint.Style.STROKE); // 선이 그려지도록
        paint.setStrokeWidth(10f); // 선의 굵기 지정
		mView = this;
    }
}
