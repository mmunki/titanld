package io.github.mmunki.titanld

import android.Manifest
import android.app.Activity
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color as AndroidColor
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.SystemBarStyle
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.blur
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import top.yukonga.miuix.kmp.basic.Button
import top.yukonga.miuix.kmp.basic.ButtonDefaults
import top.yukonga.miuix.kmp.basic.Card
import top.yukonga.miuix.kmp.basic.CardDefaults
import top.yukonga.miuix.kmp.basic.Icon
import top.yukonga.miuix.kmp.basic.IconButton
import top.yukonga.miuix.kmp.basic.NavigationBar
import top.yukonga.miuix.kmp.basic.NavigationBarItem
import top.yukonga.miuix.kmp.basic.Scaffold
import top.yukonga.miuix.kmp.basic.SmallTitle
import top.yukonga.miuix.kmp.basic.SmallTopAppBar
import top.yukonga.miuix.kmp.basic.Text
import top.yukonga.miuix.kmp.icon.MiuixIcons
import top.yukonga.miuix.kmp.icon.extended.Home
import top.yukonga.miuix.kmp.icon.extended.Info
import top.yukonga.miuix.kmp.icon.extended.Refresh
import top.yukonga.miuix.kmp.icon.extended.Tune
import top.yukonga.miuix.kmp.overlay.OverlayDialog
import top.yukonga.miuix.kmp.theme.ColorSchemeMode
import top.yukonga.miuix.kmp.theme.MiuixTheme
import top.yukonga.miuix.kmp.theme.ThemeController
import java.io.File as JavaFile
import java.io.FileOutputStream
import java.nio.charset.StandardCharsets

private const val APP_VERSION = "3.0.0"
private const val CORE_NAME = "TitanGL 0.2.0"
private const val TITANLD_DIR = "TitanLD"
private const val CONFIG_NAME = "config.json"

private val ACCENT = Color(0xFF7C5CFF)
private val CYAN = Color(0xFF37E6D0)
private val AMBER = Color(0xFFFFC15C)
private val RED = Color(0xFFFF6E7A)
private val MUTED = Color(0xFF9CA6BC)
private val GLASS = Color(0x1FFFFFFF)
private val BAR = Color(0xD90B0E14)

private const val RECOMMENDED_CONFIG = """{
  "glVersion": "4.6",
  "maxGlslCacheSize": 64,
  "logLevel": 3,
  "forceGLESVersion": 0,
  "dropDesktopExtensions": true
}
"""

private enum class Tab(val label: String) {
    Status("Status"),
    Config("Config"),
    About("About"),
}

private data class UiStatus(
    val hasStorage: Boolean,
    val configExists: Boolean,
    val configText: String,
    val abi: String,
    val libReady: Boolean,
)

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge(
            statusBarStyle = SystemBarStyle.dark(AndroidColor.TRANSPARENT),
            navigationBarStyle = SystemBarStyle.dark(AndroidColor.TRANSPARENT),
        )
        setContent {
            MiuixTheme(controller = remember { ThemeController(ColorSchemeMode.Dark) }) {
                TitanLdApp(this)
            }
        }
    }
}

@Composable
private fun TitanLdApp(activity: MainActivity) {
    var tab by rememberSaveable { mutableStateOf(Tab.Status) }
    var status by remember { mutableStateOf(loadStatus(activity)) }
    var showConfig by rememberSaveable { mutableStateOf(false) }
    var showLicence by rememberSaveable { mutableStateOf(false) }
    var showApplyConfirm by rememberSaveable { mutableStateOf(false) }
    var errorMessage by remember { mutableStateOf<String?>(null) }
    val context = LocalContext.current

    fun refresh() {
        status = loadStatus(activity)
    }

    fun seedIfMissing() {
        if (status.hasStorage && !status.configExists) {
            runCatching { TitanStorage.writeRecommended(activity) }
            status = loadStatus(activity)
        }
    }

    val legacyPermissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions(),
    ) {
        refresh()
        seedIfMissing()
    }

    fun requestStorage() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            try {
                activity.startActivity(
                    Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION)
                        .setData(Uri.parse("package:${activity.packageName}")),
                )
            } catch (_: Exception) {
                try {
                    activity.startActivity(Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION))
                } catch (_: Exception) {
                    Toast.makeText(
                        activity,
                        "Open Settings > Apps > TitanLD > Permissions and allow file access.",
                        Toast.LENGTH_LONG,
                    ).show()
                }
            }
        } else {
            legacyPermissionLauncher.launch(
                arrayOf(
                    Manifest.permission.WRITE_EXTERNAL_STORAGE,
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                ),
            )
        }
    }

    fun applyConfig(replace: Boolean) {
        if (!status.hasStorage) {
            Toast.makeText(activity, "Grant storage access first.", Toast.LENGTH_LONG).show()
            requestStorage()
            return
        }
        if (!replace && status.configExists) {
            showApplyConfirm = true
            return
        }
        try {
            TitanStorage.writeRecommended(activity)
            status = loadStatus(activity)
            Toast.makeText(activity, "TitanGL settings written.", Toast.LENGTH_LONG).show()
        } catch (e: Exception) {
            errorMessage = e.message ?: e.toString()
        }
    }

    LaunchedEffect(Unit) {
        refresh()
        seedIfMissing()
    }

    DisposableEffect(activity) {
        val observer = LifecycleEventObserver { _, event ->
            if (event == Lifecycle.Event.ON_RESUME) {
                refresh()
                seedIfMissing()
            }
        }
        activity.lifecycle.addObserver(observer)
        onDispose { activity.lifecycle.removeObserver(observer) }
    }

    Box(modifier = Modifier.fillMaxSize()) {
        AuroraBackdrop()
        Scaffold(
            containerColor = Color.Transparent,
            topBar = {
                SmallTopAppBar(
                    title = "TitanLD",
                    subtitle = "v$APP_VERSION  ·  $CORE_NAME",
                    color = BAR,
                    actions = {
                        IconButton(onClick = { refresh() }) {
                            Icon(MiuixIcons.Refresh, contentDescription = "Refresh status", tint = CYAN)
                        }
                    },
                )
            },
            bottomBar = {
                NavigationBar(color = BAR) {
                    NavigationBarItem(
                        selected = tab == Tab.Status,
                        onClick = { tab = Tab.Status },
                        icon = MiuixIcons.Home,
                        label = Tab.Status.label,
                    )
                    NavigationBarItem(
                        selected = tab == Tab.Config,
                        onClick = { tab = Tab.Config },
                        icon = MiuixIcons.Tune,
                        label = Tab.Config.label,
                    )
                    NavigationBarItem(
                        selected = tab == Tab.About,
                        onClick = { tab = Tab.About },
                        icon = MiuixIcons.Info,
                        label = Tab.About.label,
                    )
                }
            },
        ) { padding ->
            Box(modifier = Modifier.fillMaxSize().padding(padding)) {
                when (tab) {
                    Tab.Status -> StatusPage(
                        status = status,
                        onRequestStorage = { requestStorage() },
                        onApply = { applyConfig(false) },
                        onViewConfig = { showConfig = true },
                        onRefresh = { refresh() },
                    )
                    Tab.Config -> ConfigPage(
                        status = status,
                        onApply = { applyConfig(false) },
                        onViewConfig = { showConfig = true },
                    )
                    Tab.About -> AboutPage(onLicence = { showLicence = true })
                }

                if (showConfig) {
                    TextDialog(
                        title = "/sdcard/TitanLD/config.json",
                        body = if (status.configExists) status.configText else
                            "The file does not exist yet.\n\nGrant storage access and apply the recommended profile.",
                        monospace = true,
                        onDismiss = { showConfig = false },
                    )
                }

                if (showLicence) {
                    TextDialog(
                        title = "TitanLD MIT License",
                        body = TitanStorage.licenceText(context),
                        monospace = true,
                        onDismiss = { showLicence = false },
                    )
                }

                if (showApplyConfirm) {
                    OverlayDialog(
                        show = true,
                        title = "Replace existing config?",
                        summary = "/sdcard/TitanLD/config.json already exists. The recommended profile will replace it.",
                        onDismissRequest = { showApplyConfirm = false },
                    ) {
                        Row(
                            modifier = Modifier.fillMaxWidth().padding(horizontal = 18.dp, vertical = 12.dp),
                            horizontalArrangement = Arrangement.spacedBy(12.dp),
                        ) {
                            Button(
                                onClick = { showApplyConfirm = false },
                                modifier = Modifier.weight(1f),
                            ) { Text("Cancel") }
                            Button(
                                onClick = {
                                    showApplyConfirm = false
                                    applyConfig(true)
                                },
                                modifier = Modifier.weight(1f),
                                colors = ButtonDefaults.buttonColorsPrimary(),
                            ) { Text("Replace") }
                        }
                    }
                }

                errorMessage?.let { message ->
                    OverlayDialog(
                        show = true,
                        title = "Could not write the configuration",
                        summary = message,
                        onDismissRequest = { errorMessage = null },
                    ) {
                        Button(
                            onClick = { errorMessage = null },
                            modifier = Modifier.fillMaxWidth().padding(18.dp),
                        ) { Text("Close") }
                    }
                }
            }
        }
    }

}

@Composable
private fun StatusPage(
    status: UiStatus,
    onRequestStorage: () -> Unit,
    onApply: () -> Unit,
    onViewConfig: () -> Unit,
    onRefresh: () -> Unit,
) {
    Page {
        Card(
            modifier = Modifier.fillMaxWidth(),
            insideMargin = PaddingValues(20.dp),
            colors = CardDefaults.defaultColors(color = GLASS),
        ) {
            Text(
                text = "Renderer, tuned.",
                fontSize = 28.sp,
                fontWeight = FontWeight.SemiBold,
                color = MiuixTheme.colorScheme.onSurface,
            )
            Spacer(Modifier.height(6.dp))
            Text(
                text = "TitanGL turns Minecraft's desktop OpenGL into OpenGL ES, " +
                    "with a Miuix materials manager on top.",
                fontSize = 14.sp,
                lineHeight = 20.sp,
                color = MUTED,
            )
            Spacer(Modifier.height(18.dp))
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Chip("TitanGL core", CYAN)
                Chip("Miuix 0.9.4", ACCENT)
            }
        }

        SmallTitle("Status")
        Card(
            modifier = Modifier.fillMaxWidth(),
            insideMargin = PaddingValues(horizontal = 18.dp, vertical = 6.dp),
            colors = CardDefaults.defaultColors(color = GLASS),
        ) {
            StatusRow(
                "Storage access",
                if (status.hasStorage) "Granted" else "Not granted",
                if (status.hasStorage) CYAN else AMBER,
            )
            StatusRow("Renderer core", CORE_NAME, Color.White)
            StatusRow(
                "Device ABI",
                status.abi + if (status.libReady) "  ·  ready" else "  ·  missing",
                if (status.libReady) Color.White else RED,
            )
            StatusRow(
                "Profile",
                if (status.configExists) "Configured" else "Not created",
                if (status.configExists) Color.White else AMBER,
            )
        }

        Text(
            text = when {
                status.hasStorage ->
                    "TitanLD can write /sdcard/TitanLD. TitanGL reads config.json on the next game start."
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.R ->
                    "Grant All files access so TitanLD can create and tune config.json. " +
                        "Without it the renderer still loads, with built-in defaults."
                else ->
                    "Grant storage permission so TitanLD can create and tune config.json."
            },
            fontSize = 12.sp,
            lineHeight = 18.sp,
            color = MUTED,
            modifier = Modifier.padding(horizontal = 4.dp),
        )

        Button(
            onClick = onRequestStorage,
            modifier = Modifier.fillMaxWidth(),
            colors = ButtonDefaults.buttonColorsPrimary(),
        ) {
            Text(if (status.hasStorage) "Manage storage access" else "Grant storage access")
        }
        Button(onClick = onApply, modifier = Modifier.fillMaxWidth()) {
            Text("Apply recommended settings")
        }
        Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            Button(onClick = onViewConfig, modifier = Modifier.weight(1f)) {
                Text("View config")
            }
            Button(onClick = onRefresh, modifier = Modifier.weight(1f)) {
                Text("Refresh")
            }
        }

        Text(
            text = "No network permission. No launcher bundled. Select TitanLD as the renderer " +
                "in Fold Craft Launcher, Zalith Launcher or a Pojav-based launcher.",
            fontSize = 12.sp,
            lineHeight = 18.sp,
            color = MUTED,
            modifier = Modifier.padding(horizontal = 4.dp, vertical = 8.dp),
        )
    }
}

@Composable
private fun ConfigPage(status: UiStatus, onApply: () -> Unit, onViewConfig: () -> Unit) {
    Page {
        SmallTitle("Active profile")
        Card(
            modifier = Modifier.fillMaxWidth(),
            insideMargin = PaddingValues(horizontal = 18.dp, vertical = 6.dp),
            colors = CardDefaults.defaultColors(color = GLASS),
        ) {
            ConfigRow("GL version", "4.6 desktop")
            ConfigRow("GLSL cache", "64 MiB")
            ConfigRow("Log level", "Info")
            ConfigRow("Forced GLES", "Automatic")
            ConfigRow("Desktop extensions", "Dropped")
        }

        Text(
            text = if (status.configExists) status.configText else
                "config.json has not been created yet.",
            fontFamily = FontFamily.Monospace,
            fontSize = 12.sp,
            lineHeight = 18.sp,
            color = if (status.configExists) Color(0xFFD9E0F0) else MUTED,
            modifier = Modifier
                .fillMaxWidth()
                .background(Color(0x66070A12), RoundedCornerShape(14.dp))
                .padding(16.dp),
        )

        Button(
            onClick = onApply,
            modifier = Modifier.fillMaxWidth(),
            colors = ButtonDefaults.buttonColorsPrimary(),
        ) {
            Text("Write recommended config")
        }
        Button(onClick = onViewConfig, modifier = Modifier.fillMaxWidth()) {
            Text("Open full config")
        }
        Text(
            text = "Path: /sdcard/TitanLD/config.json\n" +
                "The renderer reloads it when the game starts. Existing values are replaced.",
            fontSize = 12.sp,
            lineHeight = 18.sp,
            color = MUTED,
            modifier = Modifier.padding(horizontal = 4.dp),
        )
    }
}

@Composable
private fun AboutPage(onLicence: () -> Unit) {
    val context = LocalContext.current
    Page {
        Card(
            modifier = Modifier.fillMaxWidth(),
            insideMargin = PaddingValues(20.dp),
            colors = CardDefaults.defaultColors(color = GLASS),
        ) {
            Text("TitanLD $APP_VERSION", fontSize = 24.sp, fontWeight = FontWeight.SemiBold)
            Spacer(Modifier.height(6.dp))
            Text(CORE_NAME, fontSize = 14.sp, color = CYAN)
            Spacer(Modifier.height(12.dp))
            Text(
                "TitanGL is TitanLD's own EGL/GLES renderer and GLSL translator. " +
                    "It maps desktop OpenGL contexts onto the Android ES driver and exposes " +
                    "the entry points launchers expect, including eglGetProcAddress and " +
                    "glXGetProcAddress.",
                fontSize = 13.sp,
                lineHeight = 20.sp,
                color = MUTED,
            )
        }

        SmallTitle("This release")
        Card(
            modifier = Modifier.fillMaxWidth(),
            insideMargin = PaddingValues(horizontal = 18.dp, vertical = 6.dp),
            colors = CardDefaults.defaultColors(color = GLASS),
        ) {
            ConfigRow("Manager UI", "Miuix 0.9.4 materials")
            ConfigRow("Navigation", "Status · Config · About")
            ConfigRow("Background", "Blurred aurora materials")
            ConfigRow("Renderer fix", "EGL dispatch + desktop config bits")
            ConfigRow("ABIs", "arm64-v8a · armeabi-v7a · x86 · x86_64")
        }

        Button(onClick = onLicence, modifier = Modifier.fillMaxWidth()) {
            Text("View MIT licence")
        }
        Button(
            onClick = {
                runCatching {
                    context.startActivity(
                        Intent(Intent.ACTION_VIEW, Uri.parse("https://github.com/mmunki/titanld")),
                    )
                }
            },
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text("Open project page")
        }
        Text(
            text = "TitanLD does not include a Minecraft launcher and declares no INTERNET permission. " +
                "The licence is included in the APK and in the source archive of every release.",
            fontSize = 12.sp,
            lineHeight = 18.sp,
            color = MUTED,
            modifier = Modifier.padding(horizontal = 4.dp, vertical = 8.dp),
        )
    }
}

@Composable
private fun Page(content: @Composable ColumnScope.() -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState())
            .padding(horizontal = 16.dp, vertical = 12.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        content()
        Spacer(Modifier.height(12.dp))
    }
}

@Composable
private fun StatusRow(label: String, value: String, color: Color) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(vertical = 13.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(label, fontSize = 14.sp, color = MUTED, modifier = Modifier.weight(1f))
        Text(
            text = value,
            fontSize = 14.sp,
            fontWeight = FontWeight.Medium,
            color = color,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
        )
    }
}

@Composable
private fun ConfigRow(label: String, value: String) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(label, fontSize = 14.sp, color = MUTED, modifier = Modifier.weight(1f))
        Text(value, fontSize = 14.sp, fontWeight = FontWeight.Medium, color = Color.White)
    }
}

@Composable
private fun Chip(text: String, color: Color) {
    Text(
        text = text.uppercase(),
        fontSize = 10.sp,
        fontWeight = FontWeight.SemiBold,
        letterSpacing = 1.sp,
        color = color,
        modifier = Modifier
            .background(color.copy(alpha = 0.12f), CircleShape)
            .padding(horizontal = 12.dp, vertical = 6.dp),
    )
}

@Composable
private fun TextDialog(title: String, body: String, monospace: Boolean, onDismiss: () -> Unit) {
    OverlayDialog(show = true, title = title, onDismissRequest = onDismiss) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 18.dp, vertical = 10.dp)
                .heightIn(max = 420.dp)
                .verticalScroll(rememberScrollState()),
        ) {
            SelectionContainer {
                Text(
                    text = body,
                    fontFamily = if (monospace) FontFamily.Monospace else FontFamily.Default,
                    fontSize = 12.sp,
                    lineHeight = 18.sp,
                    color = Color(0xFFD9E0F0),
                )
            }
        }
        Button(
            onClick = onDismiss,
            modifier = Modifier.fillMaxWidth().padding(horizontal = 18.dp, vertical = 12.dp),
        ) {
            Text("Close")
        }
    }
}

@Composable
private fun AuroraBackdrop() {
    val canBlur = Build.VERSION.SDK_INT >= Build.VERSION_CODES.S
    Box(modifier = Modifier.fillMaxSize().background(Color(0xFF070A12))) {
        Canvas(
            modifier = Modifier
                .fillMaxSize()
                .then(if (canBlur) Modifier.blur(70.dp) else Modifier),
        ) {
            val w = size.width
            val h = size.height
            blob(w * 0.08f, h * 0.10f, w * 0.82f, Color(0x887C5CFF))
            blob(w * 0.98f, h * 0.34f, w * 0.68f, Color(0x6637E6D0))
            blob(w * 0.18f, h * 0.86f, w * 0.78f, Color(0x55FF6E9C))
        }
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(
                    Brush.verticalGradient(
                        listOf(Color(0x22070A12), Color(0xBB070A12)),
                    ),
                ),
        )
    }
}

private fun DrawScope.blob(cx: Float, cy: Float, radius: Float, color: Color) {
    val center = Offset(cx, cy)
    drawCircle(
        brush = Brush.radialGradient(
            colors = listOf(color, Color.Transparent),
            center = center,
            radius = radius,
        ),
        radius = radius,
        center = center,
    )
}

private object TitanStorage {
    fun storageDir(): JavaFile = JavaFile(Environment.getExternalStorageDirectory(), TITANLD_DIR)

    fun configFile(): JavaFile = JavaFile(storageDir(), CONFIG_NAME)

    fun hasStorage(context: Context): Boolean =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            Environment.isExternalStorageManager()
        } else {
            context.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) ==
                PackageManager.PERMISSION_GRANTED
        }

    fun readConfig(): String = runCatching { configFile().readText() }.getOrDefault("")

    fun writeRecommended(context: Context) {
        val dir = storageDir()
        if (!dir.isDirectory && !dir.mkdirs()) {
            throw IllegalStateException("Cannot create ${dir.absolutePath}")
        }
        val bytes = RECOMMENDED_CONFIG.toByteArray(StandardCharsets.UTF_8)
        val target = configFile()
        val tmp = JavaFile(dir, "$CONFIG_NAME.titanld.tmp")
        FileOutputStream(tmp).use { out ->
            out.write(bytes)
            out.fd.sync()
        }
        if (!tmp.renameTo(target)) {
            FileOutputStream(target).use { out ->
                out.write(bytes)
                out.fd.sync()
            }
            tmp.delete()
        }
    }

    fun licenceText(context: Context): String = runCatching {
        context.assets.open("LICENSE.txt").bufferedReader(StandardCharsets.UTF_8).use { it.readText() }
    }.getOrElse {
        "MIT License, Copyright the TitanLD contributors.\nhttps://github.com/mmunki/titanld"
    }
}

private fun loadStatus(activity: Activity): UiStatus {
    val abi = Build.SUPPORTED_ABIS.firstOrNull() ?: "unknown"
    val lib = JavaFile(activity.applicationInfo.nativeLibraryDir, "libtitanld.so")
    val config = TitanStorage.configFile()
    return UiStatus(
        hasStorage = TitanStorage.hasStorage(activity),
        configExists = config.isFile,
        configText = if (config.isFile) TitanStorage.readConfig() else "",
        abi = abi,
        libReady = lib.isFile,
    )
}
