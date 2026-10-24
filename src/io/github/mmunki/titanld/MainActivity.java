package io.github.mmunki.titanld;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

public class MainActivity extends Activity {

    private static final String VERSION = "3.0.0";
    private static final String CORE = "TitanGL 0.2.0 (custom)";
    private static final String TITANLD_DIR = "TitanLD";
    private static final String TITANLD_CONFIG = "config.json";
    private static final int REQ_LEGACY_STORAGE = 4101;

    private static final int BG = Color.rgb(11, 14, 20);
    private static final int CARD = Color.rgb(20, 25, 38);
    private static final int STROKE = Color.rgb(35, 43, 61);
    private static final int TEXT = Color.rgb(238, 241, 248);
    private static final int MUTED = Color.rgb(138, 147, 166);
    private static final int ACCENT = Color.rgb(124, 92, 255);
    private static final int CYAN = Color.rgb(55, 230, 208);
    private static final int AMBER = Color.rgb(255, 193, 92);
    private static final int RED = Color.rgb(255, 110, 122);

    private TextView storageValue;
    private TextView rendererValue;
    private TextView abiValue;
    private TextView profileValue;
    private TextView storageHint;
    private Button grantButton;
    private Button applyButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(buildUi());
    }

    @Override
    protected void onResume() {
        super.onResume();
        seedIfMissing();
        refresh();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
        super.onRequestPermissionsResult(requestCode, permissions, results);
        if (requestCode == REQ_LEGACY_STORAGE) {
            refresh();
            seedIfMissing();
        }
    }

    private View buildUi() {
        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setBackgroundColor(BG);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(BG);
        root.setPadding(dp(22), dp(30), dp(22), dp(34));
        scroll.addView(root, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView title = text("TitanLD", 34, TEXT, true);
        title.setLetterSpacing(0.01f);
        root.addView(title);

        TextView subtitle = text("Minecraft Java renderer for Android", 15, MUTED, false);
        subtitle.setPadding(0, dp(4), 0, dp(2));
        root.addView(subtitle);

        TextView chip = text("CORE " + CORE + "  ·  v" + VERSION, 11, CYAN, true);
        chip.setLetterSpacing(0.08f);
        chip.setPadding(0, dp(8), 0, dp(22));
        root.addView(chip);

        root.addView(sectionLabel("STATUS"));

        LinearLayout statusCard = card();
        storageValue = addRow(statusCard, "Storage access");
        rendererValue = addRow(statusCard, "Renderer core");
        abiValue = addRow(statusCard, "Device ABI");
        profileValue = addRow(statusCard, "Profile");
        root.addView(statusCard);

        storageHint = text("", 13, MUTED, false);
        storageHint.setLineSpacing(dp(3), 1f);
        storageHint.setPadding(dp(2), dp(10), dp(2), dp(18));
        root.addView(storageHint);

        root.addView(sectionLabel("ACTIONS"));

        grantButton = button("Grant storage access", true);
        grantButton.setOnClickListener(v -> requestStorage());
        root.addView(grantButton, buttonParams());

        applyButton = button("Apply recommended settings", false);
        applyButton.setOnClickListener(v -> applyConfig(false));
        root.addView(applyButton, buttonParams());

        Button viewButton = button("View /sdcard/TitanLD/config.json", false);
        viewButton.setOnClickListener(v -> showConfig());
        root.addView(viewButton, buttonParams());

        Button licencesButton = button("Licence", false);
        licencesButton.setOnClickListener(v -> showLicences());
        root.addView(licencesButton, buttonParams());

        Button aboutButton = button("About TitanLD", false);
        aboutButton.setOnClickListener(v -> showAbout());
        root.addView(aboutButton, buttonParams());

        TextView footer = text(
                "TitanLD ships TitanGL, its own GL-to-GLES renderer core, and adds "
                        + "storage access plus a configuration manager. No network permission.",
                12, MUTED, false);
        footer.setLineSpacing(dp(3), 1f);
        footer.setPadding(dp(2), dp(20), dp(2), 0);
        root.addView(footer);

        return scroll;
    }

    private TextView sectionLabel(String s) {
        TextView tv = text(s, 11, MUTED, true);
        tv.setLetterSpacing(0.14f);
        tv.setPadding(dp(2), 0, 0, dp(8));
        return tv;
    }

    private LinearLayout card() {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setPadding(dp(18), dp(6), dp(18), dp(6));
        GradientDrawable bg = new GradientDrawable();
        bg.setColor(CARD);
        bg.setCornerRadius(dp(18));
        bg.setStroke(dp(1), STROKE);
        card.setBackground(bg);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        lp.bottomMargin = dp(6);
        card.setLayoutParams(lp);
        return card;
    }

    private TextView addRow(LinearLayout parent, String label) {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(0, dp(13), 0, dp(13));

        TextView name = text(label, 14, MUTED, false);
        row.addView(name, new LinearLayout.LayoutParams(0,
                ViewGroup.LayoutParams.WRAP_CONTENT, 1f));

        TextView value = text("—", 14, TEXT, true);
        value.setGravity(Gravity.END);
        row.addView(value, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        parent.addView(row);
        return value;
    }

    private Button button(String label, boolean primary) {
        Button b = new Button(this);
        b.setText(label);
        b.setAllCaps(false);
        b.setTextSize(15);
        b.setTypeface(Typeface.create("sans-serif-medium", Typeface.NORMAL));
        b.setTextColor(primary ? Color.WHITE : TEXT);
        b.setPadding(dp(16), 0, dp(16), 0);
        GradientDrawable bg = new GradientDrawable();
        bg.setColor(primary ? ACCENT : Color.rgb(28, 35, 51));
        bg.setCornerRadius(dp(14));
        if (!primary) {
            bg.setStroke(dp(1), STROKE);
        }
        b.setBackground(bg);
        b.setStateListAnimator(null);
        return b;
    }

    private LinearLayout.LayoutParams buttonParams() {
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(52));
        lp.bottomMargin = dp(10);
        return lp;
    }

    private TextView text(String s, int sp, int color, boolean bold) {
        TextView tv = new TextView(this);
        tv.setText(s);
        tv.setTextSize(sp);
        tv.setTextColor(color);
        if (bold) {
            tv.setTypeface(Typeface.create("sans-serif-medium", Typeface.NORMAL));
        }
        return tv;
    }

    private void refresh() {
        boolean access = hasStorage();
        storageValue.setText(access ? "Granted" : "Not granted");
        storageValue.setTextColor(access ? CYAN : AMBER);

        rendererValue.setText(CORE);
        rendererValue.setTextColor(TEXT);

        String abi = Build.SUPPORTED_ABIS.length > 0 ? Build.SUPPORTED_ABIS[0] : "unknown";
        File lib = new File(getApplicationInfo().nativeLibraryDir, "libtitanld.so");
        boolean ready = lib.isFile();
        abiValue.setText(abi + (ready ? " · ready" : " · missing"));
        abiValue.setTextColor(ready ? TEXT : RED);

        File cfg = configFile();
        profileValue.setText(cfg.isFile() ? "Configured" : "Not created");
        profileValue.setTextColor(cfg.isFile() ? TEXT : AMBER);

        grantButton.setText(access ? "Manage storage access" : "Grant storage access");
        applyButton.setEnabled(access);
        applyButton.setAlpha(access ? 1f : 0.45f);

        if (access) {
            storageHint.setText("TitanLD can write /sdcard/TitanLD. The renderer reads "
                    + "/sdcard/TitanLD/config.json on the next game start.");
            storageHint.setTextColor(MUTED);
        } else if (Build.VERSION.SDK_INT >= 30) {
            storageHint.setText("Grant \u201cAll files access\u201d so TitanLD can create and tune "
                    + "/sdcard/TitanLD/config.json. Without it the renderer still loads in the launcher, "
                    + "but it uses built-in defaults.");
            storageHint.setTextColor(AMBER);
        } else {
            storageHint.setText("Grant storage permission so TitanLD can create and tune "
                    + "/sdcard/TitanLD/config.json.");
            storageHint.setTextColor(AMBER);
        }
    }

    private boolean hasStorage() {
        if (Build.VERSION.SDK_INT >= 30) {
            return Environment.isExternalStorageManager();
        }
        return checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                == PackageManager.PERMISSION_GRANTED;
    }

    private void requestStorage() {
        if (Build.VERSION.SDK_INT >= 30) {
            try {
                Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                intent.setData(Uri.parse("package:" + getPackageName()));
                startActivity(intent);
            } catch (Exception first) {
                try {
                    startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
                } catch (Exception second) {
                    toast("Open Settings \u2192 Apps \u2192 TitanLD \u2192 Permissions and allow file access.");
                }
            }
        } else {
            requestPermissions(new String[]{
                    Manifest.permission.WRITE_EXTERNAL_STORAGE,
                    Manifest.permission.READ_EXTERNAL_STORAGE
            }, REQ_LEGACY_STORAGE);
        }
    }

    private File mgDir() {
        return new File(Environment.getExternalStorageDirectory(), TITANLD_DIR);
    }

    private File configFile() {
        return new File(mgDir(), TITANLD_CONFIG);
    }

    private void seedIfMissing() {
        if (!hasStorage() || configFile().isFile()) {
            return;
        }
        try {
            writeConfig();
            toast("Created /sdcard/TitanLD/config.json");
        } catch (Exception ignored) {
            // The status row keeps showing "Not created"; the action button reports details.
        }
    }

    private void applyConfig(boolean replace) {
        if (!hasStorage()) {
            toast("Grant storage access first.");
            requestStorage();
            return;
        }
        if (!replace && configFile().isFile()) {
            new AlertDialog.Builder(this)
                    .setTitle("Replace existing config?")
                    .setMessage("/sdcard/TitanLD/config.json already exists. TitanLD's recommended profile will "
                            + "replace it. The renderer reads this file on the next game start.")
                    .setPositiveButton("Replace", (d, w) -> {
                        try {
                            writeConfig();
                            toast("TitanGL settings written.");
                            refresh();
                        } catch (Exception e) {
                            showError(e);
                        }
                    })
                    .setNegativeButton("Cancel", null)
                    .show();
            return;
        }
        try {
            writeConfig();
            toast("TitanGL settings written.");
            refresh();
        } catch (Exception e) {
            showError(e);
        }
    }

    private void writeConfig() throws Exception {
        File dir = mgDir();
        if (!dir.isDirectory() && !dir.mkdirs()) {
            throw new Exception("Cannot create " + dir.getAbsolutePath());
        }
        byte[] bytes = recommendedConfig().getBytes(StandardCharsets.UTF_8);
        File target = configFile();
        File tmp = new File(dir, TITANLD_CONFIG + ".titanld.tmp");
        FileOutputStream out = new FileOutputStream(tmp);
        try {
            out.write(bytes);
            out.getFD().sync();
        } finally {
            out.close();
        }
        if (!tmp.renameTo(target)) {
            FileOutputStream direct = new FileOutputStream(target);
            try {
                direct.write(bytes);
                direct.getFD().sync();
            } finally {
                direct.close();
            }
            //noinspection ResultOfMethodCallIgnored
            tmp.delete();
        }
    }

    private String recommendedConfig() {
        try {
            JSONObject o = new JSONObject();
            o.put("glVersion", "4.6");
            o.put("maxGlslCacheSize", 64);
            o.put("logLevel", 3);
            o.put("forceGLESVersion", 0);
            o.put("dropDesktopExtensions", true);
            return o.toString(2) + "\n";
        } catch (Exception e) {
            return "{\n  \"glVersion\": \"4.6\",\n  \"maxGlslCacheSize\": 64,\n"
                    + "  \"logLevel\": 3,\n  \"forceGLESVersion\": 0,\n"
                    + "  \"dropDesktopExtensions\": true\n}\n";
        }
    }

    private void showConfig() {
        File cfg = configFile();
        String body;
        if (!cfg.isFile()) {
            body = "/sdcard/TitanLD/config.json does not exist yet.\n\n"
                    + "Grant storage access and tap \u201cApply recommended settings\u201d.";
        } else {
            try {
                body = readText(cfg);
            } catch (Exception e) {
                body = "Could not read " + cfg.getAbsolutePath() + "\n\n" + e;
            }
        }
        TextView tv = new TextView(this);
        tv.setText(body);
        tv.setTextSize(12);
        tv.setTextColor(TEXT);
        tv.setTypeface(Typeface.MONOSPACE);
        tv.setTextIsSelectable(true);
        tv.setPadding(dp(18), dp(12), dp(18), dp(12));
        ScrollView sv = new ScrollView(this);
        sv.addView(tv);
        new AlertDialog.Builder(this)
                .setTitle("/sdcard/TitanLD/config.json")
                .setView(sv)
                .setPositiveButton("Close", null)
                .show();
    }

    private void showAbout() {
        String msg = "TitanLD " + VERSION + "\n\n"
                + "Renderer core: " + CORE + "\n"
                + "Project: github.com/mmunki/titanld\n\n"
                + "TitanLD ships TitanGL, its own EGL/GLES renderer and GLSL translator, "
                + "plus storage access, a recommended configuration and a manager UI. It "
                + "does not include a Minecraft launcher. Select \u201cTitanLD\u201d as the "
                + "renderer in Fold Craft Launcher, Zalith Launcher or a Pojav-based "
                + "launcher.\n\n"
                + "The app declares no INTERNET permission. TitanLD's own code is MIT; the "
                + "licence is included in the APK and in the source archive of every release.";
        new AlertDialog.Builder(this)
                .setTitle("About TitanLD")
                .setMessage(msg)
                .setPositiveButton("Close", null)
                .show();
    }

    private void showLicences() {
        String body;
        try {
            InputStream in = getAssets().open("LICENSE.txt");
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buf = new byte[8192];
            int n;
            while ((n = in.read(buf)) > 0) {
                out.write(buf, 0, n);
            }
            in.close();
            body = out.toString("UTF-8");
        } catch (Exception e) {
            body = "MIT License, Copyright the TitanLD contributors.\n"
                    + "https://github.com/mmunki/titanld";
        }
        TextView tv = new TextView(this);
        tv.setText(body);
        tv.setTextSize(12);
        tv.setTextColor(TEXT);
        tv.setTypeface(Typeface.MONOSPACE);
        tv.setTextIsSelectable(true);
        tv.setPadding(dp(18), dp(12), dp(18), dp(12));
        ScrollView sv = new ScrollView(this);
        sv.addView(tv);
        new AlertDialog.Builder(this)
                .setTitle("TitanLD MIT License")
                .setView(sv)
                .setPositiveButton("Close", null)
                .show();
    }

    private void showError(Exception e) {
        new AlertDialog.Builder(this)
                .setTitle("Could not write the configuration")
                .setMessage(String.valueOf(e.getMessage()))
                .setPositiveButton("Close", null)
                .show();
    }

    private String readText(File f) throws Exception {
        FileInputStream in = new FileInputStream(f);
        try {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buf = new byte[8192];
            int n;
            while ((n = in.read(buf)) > 0) {
                out.write(buf, 0, n);
            }
            return out.toString("UTF-8");
        } finally {
            in.close();
        }
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private void toast(String s) {
        Toast.makeText(this, s, Toast.LENGTH_LONG).show();
    }
}
