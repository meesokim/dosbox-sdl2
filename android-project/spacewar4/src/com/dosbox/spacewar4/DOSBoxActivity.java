package com.dosbox.spacewar4;

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
	
}
