package com.dosbox.foxranger;

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
import android.app.Instrumentation;

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
	float px, py;
	
    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        float x = event.getX();
        float y = event.getY();
		int width = this.getWindow().getDecorView().getWidth();
        switch(event.getAction()) {
            case MotionEvent.ACTION_DOWN :
				Log.v("SDL", "x=" + x + "(" + width/2 + "),y=" + y);
				if (x > width/2) {
					SDLActivity.onNativeKeyDown(66 /*KeyEvent.KEYCODE_ENTER*/);
//					SDLActivity.onNativeKeyDown(57 /*KeyEvent.KEYCODE_LCTRL*/);
				}
				if (x > width/2 && x < width*3/4)
					SDLActivity.onNativeKeyDown(53 /*KeyEvent.KEYCODE_Y*/);
				if (x < 200 && y < 200)
					SDLActivity.onNativeKeyDown(111 /*KeyEvent.KEYCODE_ESCAPE*/);
				px = x;
				py = y;
                break;
            case MotionEvent.ACTION_MOVE :
				int len = (int)Math.sqrt((px-x)*(px-x)+(py-y)*(py-y));
				int degree = (int) (Math.atan2(px - x, py - y) * 180 / Math.PI);
				if (len > 30 & x < width / 2) { 
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
				SDLActivity.onNativeKeyDown(113);
                break;
            case MotionEvent.ACTION_UP :
				SDLActivity.onNativeKeyUp(19);
				SDLActivity.onNativeKeyUp(20);
				SDLActivity.onNativeKeyUp(21);
				SDLActivity.onNativeKeyUp(22);
				SDLActivity.onNativeKeyUp(66);
				SDLActivity.onNativeKeyUp(57);
				SDLActivity.onNativeKeyUp(53);
				SDLActivity.onNativeKeyUp(111);
///				SDLActivity.onNativeKeyUp(113);
                break;
        }
 
        return true;
    }	
	
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
    @Override
    protected void onDraw(Canvas canvas) { // 화면을 그려주는 메서드
        canvas.drawPath(path, paint); // 저장된 path 를 그려라
    }
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        float x = event.getX();
        float y = event.getY();
        switch(event.getAction()) {
            case MotionEvent.ACTION_DOWN :
				Log.v("SDL", "x=" + x + "(" + getWidth()/2 + "),y=" + y);
				if (x > getWidth()/2) {
					Instrumentation i = new Instrumentation();
					i.sendKeySync(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER));	
					i.sendKeySync(new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_ENTER));	
					SDLActivity.onNativeKeyDown(66 /*KeyEvent.KEYCODE_ENTER*/);
					SDLActivity.onNativeKeyDown(57 /*KeyEvent.KEYCODE_LCTRL*/);
				}
				px = x;
				py = y;
                break;
            case MotionEvent.ACTION_MOVE :
//              path.lineTo(x, y); // 자취에 선을 그려라
				if ((px - x) * (px - x) > 100 && ((py - y) * (py - y)) > 100) { 
					double degree = Math.atan2(px - x, py - y) * 180 / Math.PI;
					if (degree > 0 && degree < 45) {
						SDLActivity.onNativeKeyDown(22 /*KeyEvent.RIGHT*/);
						SDLActivity.onNativeKeyUp(21 /*KeyEvent.LEFT*/);
					}	
					if (degree >= 45 && degree < 135) {
						SDLActivity.onNativeKeyDown(19 /*KeyEvent.UP*/);
						SDLActivity.onNativeKeyUp(21 /*KeyEvent.Down*/);
					}
					if (degree >= 135 && degree < 180+45) {
						SDLActivity.onNativeKeyDown(21 /*KeyEvent.LEFT*/);
						SDLActivity.onNativeKeyUp(22 /*KeyEvent.RIGHT*/);
					}
					if (degree > 180+45 && degree < 360) {
						SDLActivity.onNativeKeyDown(21 /*KeyEvent.Down*/);
						SDLActivity.onNativeKeyUp(19 /*KeyEvent.UP*/);
					}
					px = x;
					py = y;
				}
                break;
            case MotionEvent.ACTION_UP :
				SDLActivity.onNativeKeyUp(19);
				SDLActivity.onNativeKeyUp(20);
				SDLActivity.onNativeKeyUp(21);
				SDLActivity.onNativeKeyUp(22);
				SDLActivity.onNativeKeyUp(66 /*KeyEvent.KEYCODE_ENTER*/);
				SDLActivity.onNativeKeyUp(57 /*KeyEvent.KEYCODE_LCTRL*/);
                break;
        }
        invalidate(); // 화면을 다시그려라
 
        return true;
    }
	int oldHeight, oldWidth;
	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		final int oldHeight = getHeight();
		final int oldWidth = getWidth();

		getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
			@Override
			public void onGlobalLayout() {
				if (mView.getHeight() != oldHeight && mView.getWidth() != oldWidth) {
					mView.getViewTreeObserver().removeOnGlobalLayoutListener(this);
					//mView now has the correct dimensions, continue with your stuff
				}
			}
		});		
		super.onConfigurationChanged(newConfig);
		
		// Checks the orientation of the screen
		if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
			Toast.makeText(getContext(), "landscape:"+getWidth() + "x" + getHeight(), Toast.LENGTH_SHORT).show();
		} else if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT){
			Toast.makeText(getContext(), "portrait"+getWidth() + "x" + getHeight(), Toast.LENGTH_SHORT).show();
		}
	}
}
