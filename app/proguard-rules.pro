# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}

# Uncomment this to preserve the line number information for
# debugging stack traces.
#-keepattributes SourceFile,LineNumberTable

# If you keep the line number information, uncomment this to
# hide the original source file name.
#-renamesourcefileattribute SourceFile

-keep,includedescriptorclasses class org.mattvchandler.a2050.MainActivity {
    void game_win(int, boolean);
    void game_over(int, boolean);
    void game_pause();
    void achievement(int);
}

-keepclasseswithmembers class org.mattvchandler.a2050.MainActivity$DispData {
    public <fields>;
}

-keepclasseswithmembers class androidx.databinding.ObservableInt {
    void set(int);
}
-keepclasseswithmembers class androidx.databinding.ObservableFloat {
    void set(float);
}

-keepclasseswithmembers class androidx.core.content.ContextCompat {
    int getColor(android.content.Context, int);
}
-keepclasseswithmembers class org.mattvchandler.a2050.R$array {
    public static <fields>;
}
-keepclasseswithmembers class org.mattvchandler.a2050.R$color {
    public static <fields>;
}
