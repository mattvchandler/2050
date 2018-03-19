package org.mattvchandler.a2050;

import android.opengl.GLSurfaceView;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.TextView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL;
import javax.microedition.khronos.opengles.GL10;

import static android.opengl.GLES30.*;

public class MainActivity extends AppCompatActivity
{
    // Used to load the 'native-lib' library on application startup.
    static
    {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        GLSurfaceView glSurfaceView = (GLSurfaceView)findViewById(R.id.gl_surface_view);

        glSurfaceView.setEGLContextClientVersion(3);
        glSurfaceView.setRenderer(new Renderer());

        glSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    public native void clear();

    class Renderer implements GLSurfaceView.Renderer
    {
        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config)
        {
            glClearColor(0.0f, 1.0f, 1.0f, 0.0f);
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height)
        {

        }

        @Override
        public void onDrawFrame(GL10 gl)
        {
            clear();
        }
    }
}
