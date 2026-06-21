--- a/src/core/config.cpp
+++ b/src/core/config.cpp
@@
 static void from_json(const nlohmann::json& j, ExperimentConfig& exp) {
     j.at("name").get_to(exp.name);
     j.at("base").get_to(exp.base);
     
     if (j.contains("distances_km")) {
         j.at("distances_km").get_to(exp.distances_km);
     }
-    if (j.contains("attacks")) {
-        j.at("attacks").get_to(exp.attacks);
-    }
+    // Support both simple list of strings and list of objects for attacks
+    if (j.contains("attacks")) {
+        const auto& a = j.at("attacks");
+        if (a.is_array()) {
+            for (const auto& item : a) {
+                if (item.is_string()) {
+                    exp.attacks.push_back(item.get<std::string>());
+                    exp.attack_params_json.push_back("\"");
+                } else if (item.is_object()) {
+                    // store name and serialized parameters
+                    if (item.contains("name")) {
+                        exp.attacks.push_back(item.at("name").get<std::string>());
+                        exp.attack_params_json.push_back(item.dump());
+                    } else {
+                        // object without name -> store full JSON as params and empty name
+                        exp.attacks.push_back("unknown");
+                        exp.attack_params_json.push_back(item.dump());
+                    }
+                }
+            }
+        }
+    }
     if (j.contains("runs_per_point")) {
         j.at("runs_per_point").get_to(exp.runs_per_point);
     }
 }
*** End Patch