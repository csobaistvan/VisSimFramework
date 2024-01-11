# custom packages
from framework.helpers import tensorflow_helper as tfh
from framework.helpers import logging_helper as lh
from framework.helpers.dotdict import dotdict

# core packages
import itertools
from tabulate import tabulate
import json
from pathlib import Path

# data & machine learning packages
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib
import matplotlib.pyplot as plt
from tqdm import tqdm

# local packages
from framework.enums import TrainingType

# create a new, module-level logger
logger = lh.get_main_module_logger()

# object for analyzing a trained network
class TrainedNetworkAnalyzer(object):
    def __init__(self, config, data_generator, model_builder_class):
        self.config = config
        self.network_params = self.config.network
        self.analyze_params = self.config.analyze
        self.data_generator = data_generator
        self.model_builder_class = model_builder_class

    # creates the plot for an analyze sample
    def _create_analyze_plot(self, model_name, data, dataset_name, metric_name, drange, nbins, out_folders):
        def process_word(word):
            base = word[0:3]
            while word[len(base)] not in 'aeiou':
                base = word[0:len(base) + 1]
            return base

        # get the short and abbreviated names
        metric_short = '_'.join(map(process_word, metric_name.split(' ')))
        metric_abbrev = ''.join(word[0].lower() for word in metric_name.split(' '))

        # set the theme used by seaborn
        sns.set_theme(rc={ 'figure.dpi': 200.0 })

        # plot the mean absolute error hist.
        g = sns.displot(
            data=data,
            kind='hist',
            bins=np.linspace(drange[0], drange[1], nbins),
            stat='probability',
            legend=False)

        # set the plot title
        title_text = '{metric} for "{dataset}"'.format(metric=metric_name, dataset=dataset_name)
        title = g.fig.axes[0].set_title(title_text)

        # set the plot legend
        legend = g.fig.axes[0].legend(labels=data.columns, ncol=1, fontsize='xx-small',
            bbox_to_anchor=(1.0, 0.5), loc='center left')

        # save the figure
        if self.analyze_params.plots.save:
            for out_params in out_folders:
                filename = out_params.fname_plot.format(
                    out_folder=out_params.folder,
                    model=model_name,
                    dataset=dataset_name, 
                    metric=metric_short.lower())
                g.savefig(
                    fname=filename, 
                    format='png', 
                    dpi=self.analyze_params.plots.save_dpi)
        
        # show the figure
        if self.analyze_params.plots.show:
            if matplotlib.get_backend() == 'Qt5Agg':
                fig_manager = plt.get_current_fig_manager()
                fig_manager.window.showMaximized()
            plt.tight_layout()
            plt.show()

    def _extract_columns(self, x, y, column_ids):
        x_cols = x[:, column_ids[0]]
        y_cols = y[:, column_ids[1]]
        return np.concatenate([x_cols, y_cols], axis=1)

    def _load_transform_network(self, network_folder):
        if tfh.keras_model_exists(network_folder, 'h5'):
            return tfh.restore_trained_keras_model(network_folder, 'h5')
        elif tfh.keras_model_checkpoint_exists(network_folder):
            return tfh.restore_keras_model_from_checkpoint(network_folder)
        raise RuntimeError('No previously trained network found in folder {}', network_folder)

    def _transform_network_outputs(self, test_data, x, y, y_hat):
        # default to the results as a PD dataframe
        y_true_df = pd.DataFrame(y, columns=test_data.target_names)
        y_pred_df = pd.DataFrame(y_hat, columns=test_data.target_names)

        # transform the input and output data via the loss network
        if 'transform_network' in test_data.network_params.loss:
            logger.info('Transforming predictions via the loss-transformer network "{}"...', test_data.loss_transform_network)

            # load the network
            network = self._load_transform_network(test_data.loss_transform_network)

            # transform the predictions
            y_pred = self._extract_columns(x, y_hat, test_data.loss_transform_network_input_cols)
            y_pred = network.predict(x=y_pred, batch_size=test_data.batch_size, verbose=1)
            y_true = self._extract_columns(x, y, test_data.loss_function_input_cols)
            x_names = [test_data.feature_names[i] for i in test_data.loss_function_input_cols[0]]
            y_names = [test_data.target_names[i] for i in test_data.loss_function_input_cols[1]]
            column_names = x_names + y_names

            y_pred_df = pd.DataFrame(y_pred, columns=column_names)
            y_true_df = pd.DataFrame(y_true, columns=column_names)

        # return the resulting dataframes
        return (y_pred_df, y_true_df)

    # tests the performance of the parameter network on the parameter dataset
    def _test_network_on_dataset(self, model, test_data, dataset_name):
        logger.info('Testing network performance on the {} dataset...', dataset_name)

        # obtain the network's name
        model_name = tfh.get_network_name(
            data_generator=self.config.data_generator, 
            network_params=test_data.network_params)

        # list of folders where the data should be put
        out_folders = [
            dotdict({ 
                'folder': test_data.network_folder + '/_analyze/' + model_name, 
                'fname_tabular': '{out_folder}/{dataset}.txt', 
                'fname_plot': '{out_folder}/{dataset}_{metric}.png',
                'fname_outlier': '{out_folder}/outliers/{dataset}_{suffix}.txt',
            }),
            dotdict({ 
                'folder': test_data.model_folder + '/_analyze/', 
                'fname_tabular': '{out_folder}/{dataset}.txt', 
                'fname_plot': '{out_folder}/{dataset}_{metric}.png',
                'fname_outlier': '{out_folder}/outliers/{dataset}_{suffix}.txt',
            })
        ]
        
        # ensure that the output folders exist
        for out_params in out_folders:
            Path(out_params.folder).mkdir(parents=True, exist_ok=True)

        # write out a summary
        for out_params in out_folders:
            model.write_summary(
                training_data=test_data, 
                out_folder=out_params.folder)

        # predict via the network
        if dataset_name == 'full':
            test_x = test_data.features.to_numpy(dtype=tfh.float_np)
            test_y = test_data.targets.to_numpy(dtype=tfh.float_np)
        else:
            test_x = getattr(test_data, 'x_{dataset}'.format(dataset=dataset_name))
            test_y = getattr(test_data, 'y_{dataset}'.format(dataset=dataset_name))
        test_y_hat = model.predict(test_data, test_x)

        # transform the outputs, if necessary
        y_pred_df, y_true_df = self._transform_network_outputs(test_data, test_x, test_y, test_y_hat)

        # min-max normalization
        if self.analyze_params.data.normalization == 'minmax_positive' or self.analyze_params.data.normalization == 'minmax':
            y_min = test_data.targets.min()
            y_max = test_data.targets.max()
            y_true_df = (y_true_df - y_min) / (y_max - y_min)
            y_pred_df = (y_pred_df - y_min) / (y_max - y_min)

        elif self.analyze_params.data.normalization == 'minmax_symmetric':
            y_min = test_data.targets.min()
            y_max = test_data.targets.max()
            y_true_df = ((y_true_df - y_min) / (y_max - y_min)) * 2 - 1
            y_pred_df = ((y_pred_df - y_min) / (y_max - y_min)) * 2 - 1

        # standardization normalization
        elif self.analyze_params.data.normalization == 'standardize':
            y_mean = test_data.targets.mean()
            y_std = test_data.targets.std()
            y_true_df = (y_true_df - y_mean) / y_std
            y_pred_df = (y_pred_df - y_mean) / y_std

        # compute the mean output values
        mtrue_per_feature = y_true_df.abs().mean(axis=0)
        mpred_per_feature = y_pred_df.abs().mean(axis=0)
        stdtrue_per_feature = y_true_df.abs().std(axis=0)
        stdpred_per_feature = y_pred_df.abs().std(axis=0)

        # compute the per-sample metrics
        ae_per_sample = (y_true_df - y_pred_df).abs()
        se_per_sample = (y_true_df - y_pred_df).pow(2)
        ape_per_sample = ((y_true_df - y_pred_df) / y_true_df).abs() * 100.0
        spe_per_sample = ((y_true_df - y_pred_df) / mtrue_per_feature).abs() * 100.0
        
        # compute the per-feature metrics
        mae_per_feature = ae_per_sample.mean(axis=0)
        mse_per_feature = se_per_sample.mean(axis=0)
        rmse_per_feature = se_per_sample.mean(axis=0).pow(0.5)
        mape_per_feature = ape_per_sample.mean(axis=0)
        mspe_per_feature = spe_per_sample.mean(axis=0)
        max_err_per_feature = ae_per_sample.max(axis=0)
        max_perc_err_per_feature = ape_per_sample.max(axis=0)
        
        # mean and maximum values
        mtrue = y_true_df.abs().stack().mean()
        mpred = y_pred_df.abs().stack().mean()
        stdtrue = y_true_df.abs().stack().std()
        stdpred = y_pred_df.abs().stack().std()
        mae = mae_per_feature.mean()
        mse = mse_per_feature.mean()
        rmse = rmse_per_feature.mean()
        mape = mape_per_feature.mean()
        mspe = mspe_per_feature.mean()
        mmax_err = max_err_per_feature.max()
        mmax_perr = max_perc_err_per_feature.max()
        ae_quantile = ae_per_sample.stack().quantile(self.analyze_params.metrics.quantile)
        se_quantile = se_per_sample.stack().quantile(self.analyze_params.metrics.quantile)
        ape_quantile = ape_per_sample.stack().quantile(self.analyze_params.metrics.quantile)
        spe_quantile = spe_per_sample.stack().quantile(self.analyze_params.metrics.quantile)

        # place the columns in a dictionary
        metrics = {
            "Target name": itertools.chain([ 'Average' ], y_true_df.keys().tolist()), 
            "Mean": itertools.chain([ mtrue ], mtrue_per_feature.values), 
            "Mean'": itertools.chain([ mpred ], mpred_per_feature.values), 
            "Std": itertools.chain([ stdtrue ], stdtrue_per_feature.values), 
            "Std'": itertools.chain([ stdpred ], stdpred_per_feature.values), 
            "MAE": itertools.chain([ mae ], mae_per_feature.values), 
            "MSE": itertools.chain([ mse ], mse_per_feature.values), 
            "RMSE": itertools.chain([ rmse ], rmse_per_feature.values), 
            "MAPE": itertools.chain([ mape ], mape_per_feature.values), 
            "MSPE": itertools.chain([ mspe ], mspe_per_feature.values),
            "Max err.": itertools.chain([ mmax_err ], max_err_per_feature.values),
            "Max perc. err.": itertools.chain([ mmax_perr ], max_perc_err_per_feature.values)
        }

        # extract the requested columns
        col_types = {
            "label": [ "Target name" ],
            "mean": [ "Mean", "Mean'" ],
            "std": [ "Std", "Std'" ],
            "mae": [ "MAE", "Max err." ],
            "mse": [ "MSE" ],
            "rmse": [ "RMSE" ],
            "mape": [ "MAPE", "Max perc. err." ],
            "mspe": [ "MSPE" ]
        }
        list_cols = list(map(lambda x: x.lower(), [ 'label' ] + self.analyze_params.tabular.metrics))
        list_cols = list(itertools.chain(*[ cols for metric, cols in col_types.items() if metric in list_cols ]))

        # put the data into a tabular format
        values = { k:v for k,v in metrics.items() if k in list_cols }
        tabulated_metrics = tabulate(
            values, 
            headers='keys',
            tablefmt=self.analyze_params.tabular.style, 
            floatfmt='.{}f'.format(self.analyze_params.tabular.precision)
        )

        # print the table
        logger.info('Per-feature metrics (over {:,} samples):\n{}', test_x.shape[0], tabulated_metrics)

        # save the tabular data
        for out_params in out_folders:
            filename = out_params.fname_tabular.format(
                out_folder=out_params.folder,
                model=model_name,
                dataset=dataset_name)
            with open(filename, 'w') as out_file:
                out_file.write(tabulated_metrics)
                out_file.write('\n')
                out_file.write(json.dumps(test_data.network_params, indent=4))

        # define the metrics to compute
        metrics = {
            "mae": dotdict({
                'metric_name': 'Absolute error',
                'data': ae_per_sample,
                'drange': (0.0, ae_quantile),
                'nbins': self.analyze_params.plots.bins
            }),
            "mse": dotdict({
                'metric_name': 'Squared error',
                'data': se_per_sample,
                'drange': (0.0, se_quantile),
                'nbins': self.analyze_params.plots.bins
            }),
            "mape": dotdict({
                'metric_name': 'Absolute percentage error',
                'data': ape_per_sample,
                'drange': (0.0, ape_quantile),
                'nbins': self.analyze_params.plots.bins
            }),
            "mspe": dotdict({
                'metric_name': 'Absolute scaled percentage error',
                'data': spe_per_sample,
                'drange': (0.0, spe_quantile),
                'nbins': self.analyze_params.plots.bins
            }),
        }

        # create the metric plots
        if self.analyze_params.plots.show or self.analyze_params.plots.save:
            for metric in self.analyze_params.plots.metrics:
                self._create_analyze_plot(
                    model_name=model_name,
                    dataset_name=dataset_name, 
                    out_folders=out_folders,
                    **metrics[metric.lower()])

        # log outliers
        # TODO: consider the param role ('eye', 'aberration', etc.) when generating MATLAB scripts
        if 'log_outliers' in self.analyze_params and (self.analyze_params.log_outliers.sum_file or self.analyze_params.log_outliers.mat_scripts):
            # extract the outliers
            headers = [ 'ID', 'Abs. error', 'Sqr. error', 'Perc. error' ]
            headers.extend(test_data.feature_names)
            headers.extend([ target + ' (true)' for target in test_data.target_names ])
            headers.extend([ target + ' (pred)' for target in test_data.target_names ])
            samples = []
            logger.info('Looking for outliers...')
            for i in tqdm(range(test_x.shape[0]), 'Evaluating samples'):
                conditions = [
                    ae_per_sample.iloc[i].mean() >= self.analyze_params.log_outliers.mean_abs_error,
                    ape_per_sample.iloc[i].mean() >= self.analyze_params.log_outliers.mean_perc_error,
                    any(ae_per_sample.iloc[i] >= self.analyze_params.log_outliers.abs_error),
                    any(ape_per_sample.iloc[i] >= self.analyze_params.log_outliers.perc_error)
                ]
                if all(conditions):
                    sample_data = []
                    sample_data.append(i)
                    sample_data.append(ae_per_sample.iloc[i].mean())
                    sample_data.append(se_per_sample.iloc[i].mean())
                    sample_data.append(ape_per_sample.iloc[i].mean())
                    sample_data.extend(test_x[i])
                    sample_data.extend(test_y.to_list())
                    sample_data.extend(test_y_hat.to_list())
                    samples.append(sample_data)

                    if self.analyze_params.log_outliers.mat_scripts:
                        for out_params in out_folders:
                            filename = out_params.fname_outlier.format(
                                out_folder=out_params.folder,
                                model=model_name,
                                dataset=dataset_name,
                                suffix=i)
                            Path(filename).parent.mkdir(parents=True, exist_ok=True)
                            with open(filename, 'w') as out_file:
                                out_file.write('% features: \n')
                                features_domain = 'eye'
                                targets_domain = 'aberrations'
                                for xi, feature in enumerate(test_data.feature_names):
                                    out_file.write('{domain}.{feature} = {value};\n'.format(domain=features_domain, feature=feature, value=test_x[i][xi]))
                                out_file.write('% targets: \n')
                                for xi, target in enumerate(test_data.target_names):
                                    out_file.write('{domain}.{target} = {value};\n'.format(domain=targets_domain, target=target, value=test_y[i][xi]))
                                out_file.write('targets_true = {targets};\n'.format(targets=y_true_df.iloc[i].to_list()))
                                out_file.write('targets_pred = {targets};\n'.format(targets=y_pred_df.iloc[i].to_list()))
                                out_file.write('abs_error = {val};\n'.format(val=ae_per_sample.iloc[i].to_list()))
                                out_file.write('sqr_error = {val};\n'.format(val=se_per_sample.iloc[i].to_list()))
                                out_file.write('perc_error = {val};\n'.format(val=ape_per_sample.iloc[i].to_list()))
                                out_file.write('scaled_perc_error = {val};\n'.format(val=spe_per_sample.iloc[i].to_list()))
                                out_file.write('mean_abs_error = {val};\n'.format(val=ae_per_sample.iloc[i].mean()))
                                out_file.write('mean_sqr_error = {val};\n'.format(val=se_per_sample.iloc[i].mean()))
                                out_file.write('mean_perc_error = {val};\n'.format(val=ape_per_sample.iloc[i].mean()))
                                out_file.write('mean_scaled_perc_error = {val};\n'.format(val=spe_per_sample.iloc[i].mean()))

            num_outliers = len(samples)

            # put them into a tabular data
            tabulated_metrics = ''
            if num_outliers > 0:
                tabulated_samples = tabulate(samples,  headers=headers, tablefmt='psql')

            if self.analyze_params.log_outliers.sum_file:
                for out_params in out_folders:
                    filename = out_params.fname_outlier.format(
                        out_folder=out_params.folder,
                        model=model_name,
                        dataset=dataset_name,
                        suffix='sum')
                    Path(filename).parent.mkdir(parents=True, exist_ok=True)
                    with open(filename, 'w') as out_file:
                        if len(samples) > 0:
                            out_file.write('Number of outliers: {} ({}%)\n'.format(num_outliers, 100.0 * num_outliers / test_x.shape[0]))
                            out_file.write('Outliers:\n')
                            out_file.write(tabulated_samples)
                        else:
                            out_file.write('No matching samples')

    def analyze(self):
        # instantiate the model builder
        model = self.model_builder_class(
            config=self.config, 
            data_generator=self.data_generator, 
            network_params=self.network_params)

        # create the test data
        test_data = model.prepare_training_data(
            training_type=TrainingType.Test)

        # test the network on train and eval datasets
        for dataset in self.analyze_params.data.datasets:
            self._test_network_on_dataset(
                model=model,
                test_data=test_data, 
                dataset_name=dataset)